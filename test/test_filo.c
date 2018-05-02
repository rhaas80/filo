#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <limits.h>
#include <unistd.h>

#include "mpi.h"
#include "filo.h"

int main(int argc, char* argv[])
{
  MPI_Init(&argc, &argv);

  int rank, ranks;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &ranks);

  char buf[256];
  sprintf(buf, "data from rank %d\n", rank);

  char filename[256];
  sprintf(filename, "/dev/shm/testfile_%d.out", rank);
  int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd != -1) {
    write(fd, buf, strlen(buf));
    close(fd);
  } else {
    printf("Error opening file %s: %d %s\n", filename, errno, strerror(errno));
  }

  char dest_filename[256];
  sprintf(dest_filename, "./testfile_%d.out", rank);

  filo_init();

  const char* filelist[1] = { filename };
  const char* dest_filelist[1] = { dest_filename };

  filo_flush("mapfile", NULL, 1, filelist, dest_filelist, MPI_COMM_WORLD);

  unlink(filename);

  /* fetch list of files recorded in mapfile to /dev/shm */
  int num_files;
  char** src_filelist;
  char** dst_filelist;
  filo_fetch("mapfile", NULL, "/dev/shm", &num_files, &src_filelist, &dst_filelist, MPI_COMM_WORLD);

  /* free file list returned by fetch */
  int i;
  for (i = 0; i < num_files; i++) {
    free(src_filelist[i]);
    free(dst_filelist[i]);
  }
  free(src_filelist);
  free(dst_filelist);

  filo_finalize();

  unlink(filename);

  MPI_Finalize();

  return 0;
}
