/*
 * Copyright (c) 2009, Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * Written by Adam Moody <moody20@llnl.gov>.
 * LLNL-CODE-411039.
 * All rights reserved.
 * This file is part of The Scalable Checkpoint / Restart (SCR) library.
 * For details, see https://sourceforge.net/projects/scalablecr/
 * Please also read this file: LICENSE.TXT.
*/

/* All rights reserved. This program and the accompanying materials
 * are made available under the terms of the BSD-3 license which accompanies this
 * distribution in LICENSE.TXT
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the BSD-3  License in
 * LICENSE.TXT for more details.
 *
 * GOVERNMENT LICENSE RIGHTS-OPEN SOURCE SOFTWARE
 * The Government's rights to use, modify, reproduce, release, perform,
 * display, or disclose this software are subject to the terms of the BSD-3
 * License as provided in Contract No. B609815.
 * Any reproduction of computer software, computer software documentation, or
 * portions thereof marked with this legend must also reproduce the markings.
 *
 * Author: Christopher Holguin <christopher.a.holguin@intel.com>
 *
 * (C) Copyright 2015-2016 Intel Corporation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <errno.h>

/* uint64_t */
#include <stdint.h>

/* basename/dirname */
#include <unistd.h>
#include <libgen.h>

#include "mpi.h"

#include "kvtree.h"
#include "kvtree_util.h"
#include "kvtree_mpi.h"

#include "axl.h"

#include "filo.h"

#ifdef HAVE_LIBDTCMP
#include "dtcmp.h"
#endif /* HAVE_LIBDTCMP */

#define FILO_KEY_PATH "PATH"
#define FILO_KEY_FILE "FILE"
#define FILO_KEY_SIZE "SIZE"
#define FILO_KEY_CRC  "CRC"
#define FILO_KEY_COMPLETE "COMPLETE"

static int filo_alltrue(int valid, MPI_Comm comm)
{
  int all_valid;
  MPI_Allreduce(&valid, &all_valid, 1, MPI_INT, MPI_LAND, comm);
  return all_valid;
}

/* allocate size bytes, returns NULL if size == 0,
 * calls er_abort if allocation fails */
void* filo_malloc(size_t size, const char* file, int line)
{
  void* ptr = NULL;
  if (size > 0) {
    ptr = malloc(size);
    if (ptr == NULL) {
      printf("Failed to allocate %llu bytes @ %s:%d", (unsigned long long) size, file, line);
    }
  }
  return ptr;
}
#define FILO_MALLOC(X) filo_malloc(X, __FILE__, __LINE__);

/* caller really passes in a void**, but we define it as just void* to avoid printing
 * a bunch of warnings */
void filo_free(void* p)
{
  /* verify that we got a valid pointer to a pointer */
  if (p != NULL) {
    /* free memory if there is any */
    void* ptr = *(void**)p;
    if (ptr != NULL) {
       free(ptr);
    }

    /* set caller's pointer to NULL */
    *(void**)p = NULL;
  }
}

/* print error message to stdout */
static void filo_err(const char *fmt, ...)
{
  va_list argp;
  //fprintf(stdout, "FILO %s ERROR: rank %d on %s: ", REDSET_VERSION, redset_rank, redset_hostname);
  fprintf(stdout, "FILO ERROR: ");
  va_start(argp, fmt);
  vfprintf(stdout, fmt, argp);
  va_end(argp);
  fprintf(stdout, "\n");
}

/* returns user's current mode as determine by his umask */
mode_t filo_getmode(int read, int write, int execute)
{
  /* lookup current mask and set it back */
  mode_t old_mask = umask(S_IWGRP | S_IWOTH);
  umask(old_mask);

  mode_t bits = 0;
  if (read) {
    bits |= (S_IRUSR | S_IRGRP | S_IROTH);
  }
  if (write) {
    bits |= (S_IWUSR | S_IWGRP | S_IWOTH);
  }
  if (execute) {
    bits |= (S_IXUSR | S_IXGRP | S_IXOTH);
  }

  /* convert mask to mode */
  mode_t mode = bits & ~old_mask & 0777;
  return mode;
}

/* recursively create directory and subdirectories */
int filo_mkdir(const char* dir, mode_t mode)
{
  int rc = FILO_SUCCESS;

  /* With dirname, either the original string may be modified or the function may return a
   * pointer to static storage which will be overwritten by the next call to dirname,
   * so we need to strdup both the argument and the return string. */

  /* extract leading path from dir = full path - basename */
  char* dircopy = strdup(dir);
  char* path    = strdup(dirname(dircopy));

  /* if we can read path or path=="." or path=="/", then there's nothing to do,
   * otherwise, try to create it */
  if (access(path, R_OK) < 0 &&
      strcmp(path,".") != 0  &&
      strcmp(path,"/") != 0)
  {
    rc = filo_mkdir(path, mode);
  }

  /* if we can write to path, try to create subdir within path */
  if (access(path, W_OK) == 0 && rc == FILO_SUCCESS) {
    int tmp_rc = mkdir(dir, mode);
    if (tmp_rc < 0) {
      if (errno == EEXIST) {
        /* don't complain about mkdir for a directory that already exists */
        filo_free(&dircopy);
        filo_free(&path);
        return FILO_SUCCESS;
      } else {
        printf("Creating directory: mkdir(%s, %x) path=%s errno=%d %s @ %s:%d",
          dir, mode, path, errno, strerror(errno), __FILE__, __LINE__
        );
        rc = FILO_FAILURE;
      }
    }
  } else {
    printf("Cannot write to directory: %s @ %s:%d",
      path, __FILE__, __LINE__
    );
    rc = FILO_FAILURE;
  }

  /* free our dup'ed string and return error code */
  filo_free(&dircopy);
  filo_free(&path);
  return rc;
}

int filo_init()
{
  if (AXL_Init(NULL) != AXL_SUCCESS) {
    return FILO_FAILURE;
  }
  return FILO_SUCCESS;
}

int filo_finalize()
{
  if (AXL_Finalize() != AXL_SUCCESS) {
    return FILO_FAILURE;
  }
  return FILO_SUCCESS;
}

/*
=========================================
Fetch functions
=========================================
*/

#if 0
/* for file name listed in meta, fetch that file from src_dir and store
 * a copy in dst_dir, record full path to copy in newfile, and
 * return whether operation succeeded */
static int filo_fetch_file(
  const char* dst_file,
  const char* src_dir)
{
  int rc = SCR_SUCCESS;

  /* build full path to source file */
  scr_path* path_src_file = scr_path_from_str(dst_file);
  scr_path_basename(path_src_file);
  scr_path_prepend_str(path_src_file, src_dir);
  char* src_file = scr_path_strdup(path_src_file);

  /* fetch the file */
  uLong crc;
  uLong* crc_p = NULL;
  if (scr_crc_on_flush) {
    crc_p = &crc;
  }
  rc = scr_file_copy(src_file, dst_file, scr_file_buf_size, crc_p);

  /* check that crc matches crc stored in meta */
  uLong meta_crc;
  if (scr_meta_get_crc32(meta, &meta_crc) == SCR_SUCCESS) {
    if (rc == SCR_SUCCESS && scr_crc_on_flush && crc != meta_crc) {
      rc = SCR_FAILURE;
      scr_err("CRC32 mismatch detected when fetching file from %s to %s @ %s:%d",
        src_file, dst_file, __FILE__, __LINE__
      );

      /* TODO: would be good to log this, but right now only rank 0
       * can write log entries */
      /*
      if (scr_log_enable) {
        time_t now = scr_log_seconds();
        scr_log_event("CRC32 MISMATCH", filename, NULL, &now, NULL);
      }
      */
    }
  }

  /* free path and string for source file */
  scr_free(&src_file);
  scr_path_delete(&path_src_file);

  return rc;
}

/* fetch files listed in hash into specified cache directory,
 * update filemap and fill in total number of bytes fetched,
 * returns SCR_SUCCESS if successful */
static int filo_fetch_files_list(
  const kvtree* file_list,
  const char* dir)
{
  /* assume we'll succeed in fetching our files */
  int rc = FILO_SUCCESS;

  /* assume we don't have any files to fetch */
  int my_num_files = 0;

  /* now iterate through the file list and fetch each file */
  kvtree_elem* file_elem = NULL;
  kvtree* files = kvtree_get(file_list, FILO_KEY_FILE);
  for (file_elem = kvtree_elem_first(files);
       file_elem != NULL;
       file_elem = kvtree_elem_next(file_elem))
  {
    /* get the filename */
    char* file = kvtree_elem_key(file_elem);

    /* get a pointer to the hash for this file */
    kvtree* hash = kvtree_elem_hash(file_elem);

    /* check whether we are supposed to fetch this file */
    /* TODO: this is a hacky way to avoid reading a redundancy file
     * back in under the assumption that it's an original file, which
     * breaks our redundancy computation due to a name conflict on
     * the file names */
    kvtree_elem* no_fetch_hash = kvtree_elem_get(hash, SCR_SUMMARY_6_KEY_NOFETCH);
    if (no_fetch_hash != NULL) {
      continue;
    }

    /* increment our file count */
    my_num_files++;

    /* build the destination file name */
    scr_path* path_newfile = scr_path_from_str(file);
    scr_path_basename(path_newfile);
    scr_path_prepend_str(path_newfile, dir);
    char* newfile = scr_path_strdup(path_newfile);

    /* get the file size */
    unsigned long filesize = 0;
    if (kvtree_util_get_unsigned_long(hash, FILO_KEY_SIZE, &filesize) != SCR_SUCCESS) {
      filo_err("Failed to read file size from summary data @ %s:%d",
        __FILE__, __LINE__
      );
      rc = SCR_FAILURE;

      /* free path and string */
      filo_free(&newfile);
      filo_path_delete(&path_newfile);

      break;
    }

    /* check for a complete flag */
    int complete = 1;
    if (kvtree_util_get_int(hash, FILO_KEY_COMPLETE, &complete) != SCR_SUCCESS) {
      /* in summary file, the absence of a complete flag on a file
       * implies the file is complete */
      complete = 1;
    }

    /* fetch native file, lookup directory for this file */
    char* from_dir;
    if (kvtree_util_get_str(hash, FILO_KEY_PATH, &from_dir) == SCR_SUCCESS) {
      if (scr_fetch_file(newfile, from_dir, meta) != SCR_SUCCESS) {
        /* failed to fetch file, mark it as incomplete */
        scr_meta_set_complete(meta, 0);
        rc = SCR_FAILURE;
      }
    } else {
      /* failed to read source directory, mark file as incomplete */
      scr_meta_set_complete(meta, 0);
      rc = SCR_FAILURE;
    }

    /* free path and string */
    scr_free(&newfile);
    scr_path_delete(&path_newfile);
  }

  return rc;
}
#endif

static int filo_axl(int num_files, const char** src_filelist, const char** dest_filelist)
{
  int rc = FILO_SUCCESS;

  /* TODO: allow user to name this transfer */

  /* define a transfer handle */
  int id = AXL_Create("AXL_XFER_SYNC", "transfer");
  if (id < 0) {
    filo_err("Failed to create AXL transfer handle @ %s:%d",
      __FILE__, __LINE__
    );
    rc = FILO_FAILURE;
  }

  /* add files to transfer list */
  int i;
  for (i = 0; i < num_files; i++) {
    const char* src_file  = src_filelist[i];
    const char* dest_file = dest_filelist[i];
    if (AXL_Add(id, src_file, dest_file) != AXL_SUCCESS) {
      filo_err("Failed to add file to AXL transfer handle %d: %s --> %s @ %s:%d",
        id, src_file, dest_file, __FILE__, __LINE__
      );
      rc = FILO_FAILURE;
    }
  }

  /* TODO: flow control the dispatch */

  /* kick off the transfer */
  if (AXL_Dispatch(id) != AXL_SUCCESS) {
    filo_err("Failed to dispatch AXL transfer handle %d @ %s:%d",
      id, __FILE__, __LINE__
    );
    rc = FILO_FAILURE;
  }

  /* wait for transfer to complete */
  int rc_axl = AXL_Wait(id);
  if (rc_axl != AXL_SUCCESS) {
    /* transfer failed */
    filo_err("Failed to wait on AXL transfer handle %d @ %s:%d",
      id, __FILE__, __LINE__
    );
    rc = FILO_FAILURE;
  }

  /* release the handle */
  if (AXL_Free(id) != AXL_SUCCESS) {
    filo_err("Failed to free AXL transfer handle %d @ %s:%d",
      id, __FILE__, __LINE__
    );
    rc = FILO_FAILURE;
  }

  return rc;
}

static int filo_window_width = 256;

/* fetch files specified in file_list into specified dir and update
 * filemap */
static int filo_axl_sliding_window(
  int num_files,
  const char** src_filelist,
  const char** dest_filelist,
  MPI_Comm comm)
{
  int success = FILO_SUCCESS;

  /* get our rank and number of ranks in comm */
  int rank_world, ranks_world;
  MPI_Comm_rank(comm, &rank_world);
  MPI_Comm_size(comm, &ranks_world);

  /* flow control rate of file reads from rank 0 */
  if (rank_world == 0) {
    /* fetch these files into the directory */
    if (filo_axl(num_files, src_filelist, dest_filelist) != FILO_SUCCESS) {
      success = FILO_FAILURE;
    }

    /* now, have a sliding window of w processes read simultaneously */
    int w = filo_window_width;
    if (w > ranks_world-1) {
      w = ranks_world-1;
    }

    /* allocate MPI_Request arrays and an array of ints */
    int* flags       = (int*)         FILO_MALLOC(2 * w * sizeof(int));
    MPI_Request* req = (MPI_Request*) FILO_MALLOC(2 * w * sizeof(MPI_Request));
    MPI_Status status;

    /* execute our flow control window */
    int outstanding = 0;
    int index = 0;
    int i = 1;
    while (i < ranks_world || outstanding > 0) {
      /* issue up to w outstanding sends and receives */
      while (i < ranks_world && outstanding < w) {
        /* post a receive for the response message we'll get back when
         * rank i is done */
        MPI_Irecv(&flags[index + w], 1, MPI_INT, i, 0, comm, &req[index + w]);

        /* send a start signal to this rank */
        flags[index] = success;
        MPI_Isend(&flags[index], 1, MPI_INT, i, 0, comm, &req[index]);

        /* update the number of outstanding requests */
        outstanding++;
        index++;
        i++;
      }

      /* wait to hear back from any rank */
      MPI_Waitany(w, &req[w], &index, &status);

      /* the corresponding send must be complete */
      MPI_Wait(&req[index], &status);

      /* check success code from process */
      if (flags[index + w] != FILO_SUCCESS) {
        success = FILO_FAILURE;
      }

      /* one less request outstanding now */
      outstanding--;
    }

    /* free the MPI_Request arrays */
    filo_free(&req);
    filo_free(&flags);
  } else {
    /* wait for start signal from rank 0 */
    MPI_Status status;
    MPI_Recv(&success, 1, MPI_INT, 0, 0, comm, &status);

    /* if rank 0 hasn't seen a failure, try to read in our files */
    if (success == FILO_SUCCESS) {
      /* fetch these files into the directory */
      if (filo_axl(num_files, src_filelist, dest_filelist) != FILO_SUCCESS) {
        success = FILO_FAILURE;
      }
    }

    /* tell rank 0 that we're done and send it our success code */
    MPI_Send(&success, 1, MPI_INT, 0, 0, comm);
  }

  /* determine whether all processes successfully read their files */
  if (filo_alltrue(success == FILO_SUCCESS, comm)) {
    return FILO_SUCCESS;
  }
  return FILO_FAILURE;
}

/* fetch files from parallel file system */
int filo_fetch(
  const char* filopath,
  const char* path,
  int* out_num_files,
  char*** out_src_filelist,
  char*** out_dest_filelist,
  MPI_Comm comm)
{
  int rc = FILO_SUCCESS;

  /* initialize output variables */
  *out_num_files     = 0;
  *out_src_filelist  = NULL;
  *out_dest_filelist = NULL;

  /* get the list of files to read */
  kvtree* filelist = kvtree_new();
  if (kvtree_read_scatter(filopath, filelist, comm) != KVTREE_SUCCESS) {
    kvtree_delete(&filelist);
    return FILO_FAILURE;
  }

  /* allocate list of file names */
  kvtree* files = kvtree_get(filelist, "FILE");
  int count = kvtree_size(files);
  const char** src_filelist  = (const char**) FILO_MALLOC(count * sizeof(char*));
  const char** dest_filelist = (const char**) FILO_MALLOC(count * sizeof(char*));

  /* create list of file names */
  int i = 0;
  kvtree_elem* elem;
  for (elem = kvtree_elem_first(files);
       elem != NULL;
       elem = kvtree_elem_next(elem))
  {
    /* strdup the filename into source list */
    const char* file = kvtree_elem_key(elem);
    src_filelist[i] = strdup(file);

    /* compute and strdup detination name into dest list */
    char destname[1024];
    char* file2 = strdup(file);
    char* name = basename(file2);
    snprintf(destname, sizeof(destname), "%s/%s", path, name);
    dest_filelist[i] = strdup(destname);
    filo_free(&file2);

    i++;
  }

  /* now we can finally fetch the actual files */
  int success = 1;
  if (filo_axl_sliding_window(count, src_filelist, dest_filelist, comm) != FILO_SUCCESS) {
    success = 0;
  }

#if 0
  /* free our list of file names */
  for (i = 0; i < count; i++) {
    filo_free(&src_filelist[i]);
    filo_free(&dest_filelist[i]);
  }
  filo_free(&src_filelist);
  filo_free(&dest_filelist);
#endif

  /* copy values to output variables */
  *out_num_files     = count;
  *out_src_filelist  = (char**) src_filelist;
  *out_dest_filelist = (char**) dest_filelist;

  /* free the list of files */
  kvtree_delete(&filelist);

  /* check that all processes copied their file successfully */
  if (! filo_alltrue(success, comm)) {
    /* TODO: auto delete files? */
    return FILO_FAILURE;
  }

  return rc;
}

/* build list of directories needed for file list (one per file) */
static int filo_create_dirs(int count, const char** dest_filelist, MPI_Comm comm)
{
  /* TODO: need to list dirs in order from parent to child */

  /* allocate buffers to hold the directory needed for each file */
  int* leader           = (int*)         FILO_MALLOC(sizeof(int)         * count);
  const char** dirs     = (const char**) FILO_MALLOC(sizeof(const char*) * count);
  uint64_t* group_id    = (uint64_t*)    FILO_MALLOC(sizeof(uint64_t)    * count);
  uint64_t* group_ranks = (uint64_t*)    FILO_MALLOC(sizeof(uint64_t)    * count);
  uint64_t* group_rank  = (uint64_t*)    FILO_MALLOC(sizeof(uint64_t)    * count);

  /* lookup directory from meta data for each file */
  int i;
  for (i = 0; i < count; i++) {
    /* extract directory from filename */
    const char* filename = dest_filelist[i];
    char* path = strdup(filename);
    dirs[i] = strdup(dirname(path));
    filo_free(&path);

    /* lookup original path where application wants file to go */
#ifndef HAVE_LIBDTCMP
    /* if we don't have DTCMP,
     * then we'll just issue a mkdir for each file, lots of extra
     * load on the file system, but this works */
    leader[i] = 1;
#else
    /* we'll use DTCMP to select one leader for each directory later */
    leader[i] = 0;
#endif
  }

#ifdef HAVE_LIBDTCMP
  /* with DTCMP we identify a single process to create each directory */

  /* identify the set of unique directories */
  uint64_t groups;
  int dtcmp_rc = DTCMP_Rankv_strings(
    count, dirs, &groups, group_id, group_ranks, group_rank,
    DTCMP_FLAG_NONE, comm
  );
  if (dtcmp_rc != DTCMP_SUCCESS) {
    rc = FILO_FAILURE;
  }

  /* select leader for each directory */
  for (i = 0; i < count; i++) {
    if (group_rank[i] == 0) {
      leader[i] = 1;
    }
  }
#endif /* HAVE_LIBDTCMP */

  /* get file mode for directory permissions */
  mode_t mode_dir = filo_getmode(1, 1, 1);

  /* TODO: add flow control here */

  /* create other directories in file list */
  int success = 1;
  for (i = 0; i < count; i++) {
    if (leader[i]) {
      /* create directory */
      const char* dir = dirs[i];
      if (filo_mkdir(dir, mode_dir) != FILO_SUCCESS) {
        success = 0;
      }
      filo_free(&dir);
    }
  }

  /* free buffers */
  filo_free(&group_id);
  filo_free(&group_ranks);
  filo_free(&group_rank);
  filo_free(&dirs);
  filo_free(&leader);

  /* determine whether all leaders successfully created their directories */
  if (! filo_alltrue(success == 1, comm)) {
    return FILO_FAILURE;
  }
  return FILO_SUCCESS;
}

int filo_flush(
  const char* filopath,
  int num_files,
  const char** src_filelist,
  const char** dest_filelist,
  MPI_Comm comm)
{
  int rc = FILO_SUCCESS;

  /* build a list of files for this rank */
  int i;
  kvtree* filelist = kvtree_new();
  for (i = 0; i < num_files; i++) {
    const char* filename = dest_filelist[i];
    kvtree_set_kv(filelist, FILO_KEY_FILE, filename);
  }

  /* save our file list to disk */
  kvtree_write_gather(filopath, filelist, comm);

  /* create directories */
  rc = filo_create_dirs(num_files, dest_filelist, comm);

  /* write files (via AXL) */
  int success = 1;
  if (filo_axl_sliding_window(num_files, src_filelist, dest_filelist, comm) != FILO_SUCCESS) {
    success = 0;
  }

  /* free the list of files */
  kvtree_delete(&filelist);

  /* check that all processes copied their file successfully */
  if (! filo_alltrue(success, comm)) {
    /* TODO: auto delete files? */
    rc = FILO_FAILURE;
  }

  return rc;
}
