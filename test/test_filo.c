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
  rc = Filo_Flush("mapfile", NULL, 1, filelist, dest_filelist,
    MPI_COMM_WORLD, "pthread");

  unlink(filename);

  /* fetch list of files recorded in mapfile to /dev/shm */
  int num_files;
  char** src_filelist;
  char** dst_filelist;
  /* src base path is still NULL (consistent with Filo_Flush), but the dest base path is /dev/shm*/
  rc = Filo_Fetch("mapfile", NULL, "/dev/shm", &num_files, &src_filelist,
    &dst_filelist, MPI_COMM_WORLD, "pthread");

  /* free file list returned by fetch */
  int i;
  for (i = 0; i < num_files; i++) {
    //in file name, rank precedes ".out" suffix
    int rank_from_file_name = *((strstr(dst_filelist[i], ".out"))-1) - '0';
    //assertain that the filename with consistant process marker was passed through flush/fetch
    if(rank != rank_from_file_name){
      printf ("Error in line %d, file %s, function %s.\n", __LINE__, __FILE__, __func__);
      printf("rank = %d, rank_from_file_name = %d\n", rank, rank_from_file_name);
      return TEST_FAIL;
    }
    //assertain that the file content is consistent with the process
    FILE *file = fopen(dst_filelist[i], "r");
    char *readContent = NULL;
    size_t readContent_size = 0;
    if (!file){
      printf ("Error in line %d, file %s, function %s.\n", __LINE__, __FILE__, __func__);
      printf("Error opening file %s: %d %s\n", dst_filelist[i], errno, strerror(errno));
      return TEST_FAIL;
    }
    size_t line_size = getline(&readContent, &readContent_size, file);
    if (strcmp(readContent, proc_specific_file_content) != 0){
      printf ("Error in line %d, file %s, function %s.\n", __LINE__, __FILE__, __func__);
      printf("flushed file content = %s, fetched file content = %s\n", proc_specific_file_content, readContent);
      return TEST_FAIL;
    }

    fclose(file);
    free(src_filelist[i]);
    free(dst_filelist[i]);
  }
  //MPI_COMM_NULL test
  rc = Filo_Flush("mapfile", NULL, 1, filelist, dest_filelist, MPI_COMM_NULL, "pthread");
  if(rc == FILO_SUCCESS){
    printf ("Error in line %d, file %s, function %s.\n", __LINE__, __FILE__, __func__);
    printf("Filo_Flush succeded with MPI_COMM_NULL comm parameters\n");
    return TEST_FAIL;
  }
  rc = Filo_Fetch("mapfile", NULL, "/dev/shm", &num_files, &src_filelist, &dst_filelist, MPI_COMM_NULL, "pthread");
  if(rc == FILO_SUCCESS){
    printf ("Error in line %d, file %s, function %s.\n", __LINE__, __FILE__, __func__);
    printf("Filo_Fetch succeded with MPI_COMM_NULL comm parameters\n");
    return TEST_FAIL;
  }

  free(src_filelist);
  free(dst_filelist);

  rc = Filo_Finalize();

  unlink(filename);

  MPI_Finalize();

  return rc;
}
