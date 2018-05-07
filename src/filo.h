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

#ifndef FILO_H
#define FILO_H

#include "mpi.h"

#define FILO_SUCCESS (0)
#define FILO_FAILURE (1)

/* intialize the library */
int Filo_Init();

/* shut down the library */
int Filo_Finalize();

/* given a pointer to a filo file at filopath that was written by
 * filo_flush, copy files from their current location to path,
 * returns the number of files copied and the full path to the
 * source and destination path for each file, prepends basepath
 * to source path if basepath is not NULL
 *
 * the length of each file list is num_files
 *
 * the caller is responsible for freeing each file name in the
 * output file lists, as well as the list object itself using
 * basic free() calls:
 * 
 *    for (int i = 0; i < num_files; i++) {
 *      free(src_filelist[i]);
 *      free(dest_filelist[i]);
 *    }
 *    free(src_filelist);
 *    free(dest_filelist);
*/
int Filo_Fetch(
  const char* filopath,  /* IN  - path to filo metadata file */
  const char* basepath,  /* IN  - prefix path to prepend to each source file */
  const char* path,      /* IN  - path to copies files to */
  int* num_files,        /* OUT - number of files copied */
  char*** src_filelist,  /* OUT - full source path of each file copied */
  char*** dest_filelist, /* OUT - full destination path of each file copied */
  MPI_Comm comm          /* IN  - communicator used for coordination and flow control */
);

/* copies the list of source files to their corresponding
 * destination paths, records list of files in a filo metadata
 * file written to filopath, if basepath is not NULL, each
 * file is recorded relative to basepath, which makes a directory
 * of such files relocatable */
int Filo_Flush(
  const char* filopath,       /* IN - path to filo metadata file */
  const char* basepath,       /* IN - store metadata file names relative to basepath if not NULL */
  int num_files,              /* IN - number of files in source and dest lists */
  const char** src_filelist,  /* IN - list of source paths */
  const char** dest_filelist, /* IN - list of destination paths */
  MPI_Comm comm               /* IN - communicator used for coordination and flow control */
);

int Filo_Flush_start(
  const char* filopath,       /* IN - path to filo metadata file */
  const char* basepath,       /* IN - store metadata file names relative to basepath if not NULL */
  int num_files,              /* IN - number of files in source and dest lists */
  const char** src_filelist,  /* IN - list of source paths */
  const char** dest_filelist, /* IN - list of destination paths */
  MPI_Comm comm               /* IN - communicator used for coordination and flow control */
);

int Filo_Flush_test(
  const char* filopath,       /* IN - path to filo metadata file */
  MPI_Comm comm               /* IN - communicator used for coordination and flow control */
);

int Filo_Flush_wait(
  const char* filopath,       /* IN - path to filo metadata file */
  MPI_Comm comm               /* IN - communicator used for coordination and flow control */
);

int Filo_Flush_stop(
  MPI_Comm comm               /* IN - communicator used for coordination and flow control */
);

#endif /* FILO_H */
