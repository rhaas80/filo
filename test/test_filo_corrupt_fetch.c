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

#define TEST_PASS (0)
#define TEST_FAIL (1)

int main(int argc, char* argv[])
{
  int rc = TEST_PASS;
  MPI_Init(&argc, &argv);
  int rank, ranks;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &ranks);

  char proc_specific_file_content[256];
  sprintf(proc_specific_file_content, "data from rank %d\n", rank);
  char filename[256];
  sprintf(filename, "/dev/shm/testfile_%d.out", rank);
  int fd = open(filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
  if (fd != -1) {
    write(fd, proc_specific_file_content, strlen(proc_specific_file_content));
    close(fd);
  } else {
    printf ("Error in line %d, file %s, function %s.\n", __LINE__, __FILE__, __func__);
    printf("Error opening file %s: %d %s\n", filename, errno, strerror(errno));
    return TEST_FAIL;
  }

  char dest_filename[256];
  sprintf(dest_filename, "./testfile_%d.out", rank);

  rc = Filo_Init();

  const char* filelist[1] = { filename };
  const char* dest_filelist[1] = { dest_filename };

  /* base path for storage is NULL, so destination files will be written to the local dir*/
  rc = Filo_Flush("mapfile", NULL, 1, filelist, dest_filelist, MPI_COMM_WORLD, NULL);

  //remove one of the flush destination files. This should result in Filo_Fetch returning error on ALL processes.
  if(rank == 1) unlink(dest_filename);

  unlink(filename);

  /* fetch list of files recorded in mapfile to /dev/shm */
  int num_files;
  char** src_filelist;
  char** dst_filelist;
  /* src base path is still NULL (consistent with Filo_Flush), but the dest base path is /dev/shm*/
  int filo_ret = Filo_Fetch("mapfile", NULL, "/dev/shm", &num_files, &src_filelist, &dst_filelist, MPI_COMM_WORLD, NULL);
  //check if the return value is error -- which is the correct behavior -- otherwise return fail
  if(filo_ret == 0){
    printf ("Error in line %d, file %s, function %s.\n", __LINE__, __FILE__, __func__);
    printf("Error: Filo_Fetch should fail because rank 1 destination file was removed pre- fetch. rank = %d, filo_ret = %d\n", rank, filo_ret);
    return TEST_FAIL;
  }

  MPI_Finalize();

  return rc;
}
