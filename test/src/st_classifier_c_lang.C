#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"
#include "g_fs_stat_c_wrap.h"

#define one_megabyte 1024*1024
#define one_gigabyte 1024*1024*1024

int main(int argc, char *argv[])
{
  
  int rank = -1;
  int size = -1;
  int rc = -1;
  int match_array_size = -1;
  int i = 0;
  matched_entry_t *match_array = NULL;

  //
  // Initialize the MPI Communication Fabric
  //
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if ( (rc = g_fs_status_initialize()) == -1) {
    fprintf(stderr, "ST_CLASSIFIER_C_LANG: g_fs_status_initialize returned an error\n");
    MPI_Finalize();
    return EXIT_FAILURE;
  }

  rc = g_fs_status_provide_best_fs(one_megabyte, 0, &match_array, &match_array_size);

  for (i=0; i < match_array_size; ++i) {
    if (!rank) {
      fprintf(stdout, "ST_CLASSIFIER_C_LANG: path: %s with score(%d)\n", 
  	       match_array[i].mount_point_path,
	       match_array[i].score);
    } 
    free(match_array[i].mount_point_path);
  }

  if (match_array_size > 0) {
    free (match_array);
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}
