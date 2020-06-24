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

/** initialize the library */
int Filo_Init(void);

/** shut down the library */
int Filo_Finalize(void);

/** Set a FILO config parameter */
int Filo_Config(char *config_str);

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
