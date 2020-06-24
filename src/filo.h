#ifndef FILO_H
#define FILO_H

#include "mpi.h"

/** \defgroup filo FILO
 *  \brief File flush and fetch, with MPI
 *
 * Each process in a communicator registers a list of source and
 * destination paths. FILO then computes the union of destination
 * directories and creates them in advance, using minimal mkdir()
 * calls. It executes AXL transfers, optionally using a sliding window
 * for flow control. It will record ownership map of which rank
 * flushed which file (in the rank2file file). This is used to fetch
 * those files back to owner ranks during a restart. */

/** \file filo.h
 *  \ingroup filo
 *  \brief File flush and fetch */

/* enable C++ codes to include this header directly */
#ifdef __cplusplus
extern "C" {
#endif

#define FILO_SUCCESS (0)
#define FILO_FAILURE (1)

#define FILO_KEY_CONFIG_FETCH_WIDTH "FETCH_WIDTH"
#define FILO_KEY_CONFIG_FLUSH_WIDTH "FLUSH_WIDTH"
/* option below are not used by FILO directly but passed on to AXL */
/* TODO: need to define defaults in FILO as discussed with Adam */
#define FILO_KEY_CONFIG_FILE_BUF_SIZE "FILE_BUF_SIZE"
#define FILO_KEY_CONFIG_DEBUG "DEBUG"
#define FILO_KEY_CONFIG_MKDIR "MKDIR"
#define FILO_KEY_CONFIG_COPY_METADATA "COPY_METADATA"

/** initialize the library */
int Filo_Init(void);

/** shut down the library */
int Filo_Finalize(void);

extern int filo_fetch_width; /**< number of processes to read files simultaneously */
extern int filo_flush_width; /**< number of processes to write files simultaneously*/
extern unsigned long filo_file_buf_size; /**< buffer size to use for file I/O operations */
extern int filo_debug; /**< if non-zero, print debug information*/
extern int filo_make_directories; /**< whether Filo should first create parent directories before transferring files */
extern int filo_copy_metadata; /**< whether file metadata should also be copied*/

/* needs to be above doxygen comment to get association right */
typedef struct kvtree_struct kvtree;

/**
 * Get/set Filo configuration values.
 *
 * The following configuration options can be set (type in parenthesis):
 *   * "FETCH_WIDTH" (int) - number of processes to read files simultaneously.
 *   * "FLUSH_WIDTH" (int) - number of processes to write files simultaneously.
 *   * "FILE_BUF_SIZE" (byte count [IN], unsigned long int [OUT]) - buffer size
 *     to chunk file transfer. Must not exceed ULONG_MAX.
 *   * "DEBUG" (int) - if non-zero, output debug information from inside
 *     Filo.
 *   * "MKDIR" (int) - whether Filo should first create parent directories
 *     before transferring files.
 *   * "COPY_METADATA" (int) - whether file metadata should also be copied.
 *   .
 * Symbolic names FILO_KEY_CONFIG_FOO are defined in filo.h and should be used
 * instead of the strings whenever possible to guard against typos in strings.
 *
 * \result If config != NULL, then return config on success.  If config == NULL
 *         (you're querying the config) then return a new kvtree on success,
 *         which must be kvtree_delete()ed by the caller. NULL on any failures.
 * \param config The new configuration. If config == NULL, then return a new
 *               kvtree with all the configuration values.
 *
 */
kvtree* Filo_Config(const kvtree* config);

/** given a pointer to a filo file at filopath that was written by
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
 *     for (int i = 0; i < num_files; i++) {
 *       free(src_filelist[i]);
 *       free(dest_filelist[i]);
 *     }
 *     free(src_filelist);
 *     free(dest_filelist);
*/
int Filo_Fetch(
  const char* filopath,  /**< [IN]  - path to filo metadata file */
  const char* basepath,  /**< [IN]  - prefix path to prepend to each source file */
  const char* path,      /**< [IN]  - path to copies files to */
  int* num_files,        /**< [OUT] - number of files copied */
  char*** src_filelist,  /**< [OUT] - full source path of each file copied */
  char*** dest_filelist, /**< [OUT] - full destination path of each file copied */
  MPI_Comm comm,         /**< [IN]  - communicator used for coordination and flow control */
  const char *axl_xfer_str  /**< [IN]  - AXL transfer type ("sync", "pthread", "bbapi", etc) */
);

/** copies the list of source files to their corresponding
 * destination paths, records list of files in a filo metadata
 * file written to filopath, if basepath is not NULL, each
 * file is recorded relative to basepath, which makes a directory
 * of such files relocatable */
int Filo_Flush(
  const char* filopath,       /**< [IN] - path to filo metadata file */
  const char* basepath,       /**< [IN] - store file names relative to basepath if not NULL */
  int num_files,              /**< [IN] - number of files in source and dest lists */
  const char** src_filelist,  /**< [IN] - list of source paths */
  const char** dest_filelist, /**< [IN] - list of destination paths */
  MPI_Comm comm,              /**< [IN] - communicator used for coordination and flow control */
  const char *axl_xfer_str  /**< [IN] - AXL transfer type ("sync", "pthread", "bbapi", etc) */
);

int Filo_Flush_start(
  const char* filopath,       /**< [IN] - path to filo metadata file */
  const char* basepath,       /**< [IN] - store metadata file names relative to basepath if not NULL */
  int num_files,              /**< [IN] - number of files in source and dest lists */
  const char** src_filelist,  /**< [IN] - list of source paths */
  const char** dest_filelist, /**< [IN] - list of destination paths */
  MPI_Comm comm,              /**< [IN] - communicator used for coordination and flow control */
  const char *axl_xfer_str    /**< [IN]  - AXL transfer type ("sync", "pthread", "bbapi", etc) */
);

int Filo_Flush_test(
  const char* filopath,       /**< [IN] - path to filo metadata file */
  MPI_Comm comm               /**< [IN] - communicator used for coordination and flow control */
);

int Filo_Flush_wait(
  const char* filopath,       /**< [IN] - path to filo metadata file */
  MPI_Comm comm               /**< [IN] - communicator used for coordination and flow control */
);

int Filo_Flush_stop(
  MPI_Comm comm               /**< [IN] - communicator used for coordination and flow control */
);

/**********************************************/
/** \name File Set Functions
 *  \brief  Write metadata files using a single process
 *  for a filo set that can be read with Filo_Fetch */
///@{

/** define a type for Filo_Set objects */
typedef void Filo_Set;

/** return a newly allocated empty set */
Filo_Set* Filo_Set_new(
  int ranks /**< [IN]  - number of ranks to be included in set */
);

/** add file to be owned by rank to set */
int Filo_Set_add(
  Filo_Set* set,   /**< [IN] - set to add file to */
  int rank,        /**< [IN] - rank owning this file */
  const char* file /**< [IN] - path to file */
);

/** generate filo metadata corresponding to set,
 * once written this metadata file can be used in Filo_Fetch */
int Filo_Set_write(
  Filo_Set* set,        /**< [IN] - set to be written to file */
  const char* filopath, /**< [IN] - path to filo metadata file */
  const char* basepath  /**< [IN] - store file names in set relative to basepath if not NULL */
);

/** delete a set, and set pointer to NULL */
int Filo_Set_delete(
  Filo_Set** pset /**< [IN]  - set to be deleted */
);
///@}

/* enable C++ codes to include this header directly */
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FILO_H */
