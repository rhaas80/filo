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

int filo_init();

int filo_finalize();

int filo_fetch(
  const char* src_path,
  const char* dest_path,
  MPI_Comm comm
);

int filo_flush(
  int num_files,
  const char** src_filelist,
  const char** dest_filelist,
  const char* dest_path,
  MPI_Comm comm
);

#endif /* FILO_H */
