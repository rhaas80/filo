#include <stdio.h>
#include <stdlib.h>
#include "filo.h"
#include "filo_internal.h"

#include "kvtree.h"
#include "kvtree_util.h"

/* helper function to check for known options */
void check_known_options(const kvtree* configured_values,
                         const char* known_options[])
{
  /* report all unknown options (typos?) */
  const kvtree_elem* elem;
  for (elem = kvtree_elem_first(configured_values);
       elem != NULL;
       elem = kvtree_elem_next(elem))
  {
    const char* key = kvtree_elem_key(elem);

    /* must be only one level deep, ie plain kev = value */
    const kvtree* elem_hash = kvtree_elem_hash(elem);
    if (kvtree_size(elem_hash) != 1) {
      printf("Element %s has unexpected number of values: %d", key,
           kvtree_size(elem_hash));
      exit(EXIT_FAILURE);
    }

    const kvtree* kvtree_first_elem_hash =
      kvtree_elem_hash(kvtree_elem_first(elem_hash));
    if (kvtree_size(kvtree_first_elem_hash) != 0) {
      printf("Element %s is not a pure value", key);
      exit(EXIT_FAILURE);
    }

    /* check against known options */
    const char** opt;
    int found = 0;
    for (opt = known_options; *opt != NULL; opt++) {
      if (strcmp(*opt, key) == 0) {
        found = 1;
        break;
      }
    }
    if (! found) {
      printf("Unknown configuration parameter '%s' with value '%s'\n",
        kvtree_elem_key(elem),
        kvtree_elem_key(kvtree_elem_first(kvtree_elem_hash(elem)))
      );
      exit(EXIT_FAILURE);
    }
  }
}

/* helper function to check option values in kvtree against expected values */
    FILO_KEY_CONFIG_FETCH_WIDTH,
    FILO_KEY_CONFIG_FLUSH_WIDTH,
    FILO_KEY_CONFIG_FILE_BUF_SIZE,
    FILO_KEY_CONFIG_DEBUG,
    FILO_KEY_CONFIG_MKDIR,
    FILO_KEY_CONFIG_COPY_METADATA,
void check_options(const int exp_fetch_width, const int exp_flush_width,
  const unsigned long exp_file_buf_width, const int exp_debug,
  const int exp_make_directories, const int exp_copy_metadata)
{
  kvtree* config = Filo_Config(NULL);
  if (config == NULL) {
    printf("Filo_Config failed\n");
    exit(EXIT_FAILURE);
  }

  int cfg_fetch_width;
  if (kvtree_util_get_int(config, FILO_KEY_CONFIG_FETCH_WIDTH, &cfg_fetch_width) !=
    KVTREE_SUCCESS)
  {
    printf("Could not get %s from Filo_Config\n",
           FILO_KEY_CONFIG_FETCH_WIDTH);
    exit(EXIT_FAILURE);
  }
  if (cfg_fetch_width != exp_fetch_width) {
    printf("Filo_Config returned unexpected value %d for %s. Expected %d.\n",
           cfg_fetch_width, FILO_KEY_CONFIG_FETCH_WIDTH,
           exp_fetch_width);
    exit(EXIT_FAILURE);
  }

  int cfg_flush_width;
  if (kvtree_util_get_int(config, FILO_KEY_CONFIG_FLUSH_WIDTH, &cfg_flush_width) !=
    KVTREE_SUCCESS)
  {
    printf("Could not get %s from Filo_Config\n",
           FILO_KEY_CONFIG_FLUSH_WIDTH);
    exit(EXIT_FAILURE);
  }
  if (cfg_flush_width != exp_flush_width) {
    printf("Filo_Config returned unexpected value %d for %s. Expected %d.\n",
           cfg_flush_width, FILO_KEY_CONFIG_FLUSH_WIDTH,
           exp_flush_width);
    exit(EXIT_FAILURE);
  }

  unsigned int cfg_file_buf_size;
  if (kvtree_util_get_unsigned_int(config, FILO_KEY_CONFIG_FILE_BUF_SIZE,
    &cfg_file_buf_size) != KVTREE_SUCCESS)
  {
    printf("Could not get %s from Filo_Config\n",
           FILO_KEY_CONFIG_FILE_BUF_SIZE);
    exit(EXIT_FAILURE);
  }
  if (cfg_file_buf_size != exp_file_buf_size) {
    printf("Filo_Config returned unexpected value %d for %s. Expected %d.\n",
           cfg_file_buf_size, FILO_KEY_CONFIG_FILE_BUF_SIZE,
           exp_file_buf_size);
    exit(EXIT_FAILURE);
  }

  int cfg_debug;
  if (kvtree_util_get_int(config, FILO_KEY_CONFIG_DEBUG, &cfg_debug) !=
    KVTREE_SUCCESS)
  {
    printf("Could not get %s from Filo_Config\n",
           FILO_KEY_CONFIG_DEBUG);
    exit(EXIT_FAILURE);
  }
  if (cfg_debug != exp_debug) {
    printf("Filo_Config returned unexpected value %d for %s. Expected %d.\n",
           cfg_debug, FILO_KEY_CONFIG_DEBUG,
           exp_debug);
    exit(EXIT_FAILURE);
  }

  int cfg_make_directories;
  if (kvtree_util_get_int(config, FILO_KEY_CONFIG_DEBUG, &cfg_make_directories) !=
    KVTREE_SUCCESS)
  {
    printf("Could not get %s from Filo_Config\n",
           FILO_KEY_CONFIG_DEBUG);
    exit(EXIT_FAILURE);
  }
  if (cfg_make_directories != exp_make_directories) {
    printf("Filo_Config returned unexpected value %d for %s. Expected %d.\n",
           cfg_make_directories, FILO_KEY_CONFIG_DEBUG,
           exp_make_directories);
    exit(EXIT_FAILURE);
  }

  int cfg_copy_metadata;
  if (kvtree_util_get_int(config, FILO_KEY_CONFIG_COPY_METADATA, &cfg_copy_metadata) !=
    KVTREE_SUCCESS)
  {
    printf("Could not get %s from Filo_Config\n",
           FILO_KEY_CONFIG_COPY_METADATA);
    exit(EXIT_FAILURE);
  }
  if (cfg_copy_metadata != exp_copy_metadata) {
    printf("Filo_Config returned unexpected value %d for %s. Expected %d.\n",
           cfg_copy_metadata, FILO_KEY_CONFIG_COPY_METADATA,
           exp_copy_metadata);
    exit(EXIT_FAILURE);
  }

  static const char* known_options[] = {
    FILO_KEY_CONFIG_FETCH_WIDTH,
    FILO_KEY_CONFIG_FLUSH_WIDTH,
    FILO_KEY_CONFIG_FILE_BUF_SIZE,
    FILO_KEY_CONFIG_DEBUG,
    FILO_KEY_CONFIG_MKDIR,
    FILO_KEY_CONFIG_COPY_METADATA,
    NULL
  };
  check_known_options(config, known_options);

  kvtree_delete(&config);
}

int main(int argc, char *argv[])
{
  int rc;
  char *conf_file = NULL;
  kvtree* filo_config_values = kvtree_new();

  MPI_Init(&argc, &argv);

  rc = Filo_Init(conf_file);
  if (rc != Filo_SUCCESS) {
    printf("Filo_Init() failed (error %d)\n", rc);
    return rc;
  }

  int old_filo_fetch_width = filo_fetch_width;
  int old_filo_flush_width = filo_flush_width;
  unsigned long old_filo_file_buf_size = filo_file_buf_size;
  int old_filo_debug = filo_debug;
  int old_filo_make_directories = filo_make_directories;
  int old_filo_copy_metadata = filo_copy_metadata;

  int new_filo_fetch_width = old_filo_fetch_width + 1;
  int new_filo_flush_width = old_filo_flush_width + 1;
  unsigned long new_filo_file_buf_size = old_filo_file_buf_size + 1;
  int new_filo_debug = !old_filo_debug;
  int new_filo_make_directories = !old_filo_make_directories;
  int new_filo_copy_metadata = !old_filo_copy_metadata;

  check_options(old_fetch_width, old_flush_width, old_file_buf_width,
                old_debug, old_make_directories, old_copy_metadata);

  /* check Filo configuration settings */
  rc = kvtree_util_set_int(filo_config_values, FILO_KEY_CONFIG_FETCH_WIDTH,
                           !old_filo_fetch_width);
  if (rc != KVTREE_SUCCESS) {
    printf("kvtree_util_set_int failed (error %d)\n", rc);
    return rc;
  }

  printf("Configuring Filo (first set of options)...\n");
  if (Filo_Config(filo_config_values) == NULL) {
    printf("Filo_Config() failed\n");
    return EXIT_FAILURE;
  }

  /* check that option was set */

  if (filo_fetch_width != !old_filo_fetch_width) {
    printf("Filo_Config() failed to set %s: %d != %d\n",
           FILO_KEY_CONFIG_FETCH_WIDTH, filo_fetch_width, new_filo_fetch_width);
    return EXIT_FAILURE;
  }

  check_options(new_fetch_width, old_flush_width, old_file_buf_width,
                old_debug, old_make_directories, old_copy_metadata);

  /* set remainder of options */
  kvtree_delete(&filo_config_values);
  filo_config_values = kvtree_new();

  rc = kvtree_util_set_int(filo_config_values, FILO_KEY_CONFIG_FLUSH_WIDTH,
                           new_filo_flush_width);
  if (rc != KVTREE_SUCCESS) {
    printf("kvtree_util_set_int failed (error %d)\n", rc);
    return rc;
  }
  rc = kvtree_util_set_unsigned_int(filo_config_values,
    FILO_KEY_CONFIG_FILE_BUF_SIZE, new_filo_file_buf_size);
  if (rc != KVTREE_SUCCESS) {
    printf("kvtree_util_set_unsigned_int failed (error %d)\n", rc);
    return rc;
  }
  rc = kvtree_util_set_int(filo_config_values, FILO_KEY_CONFIG_DEBUG,
                           new_filo_debug);
  if (rc != KVTREE_SUCCESS) {
    printf("kvtree_util_set_int failed (error %d)\n", rc);
    return rc;
  }
  rc = kvtree_util_set_int(filo_config_values, FILO_KEY_CONFIG_MKDIR,
                           new_filo_make_directories);
  if (rc != KVTREE_SUCCESS) {
    printf("kvtree_util_set_int failed (error %d)\n", rc);
    return rc;
  }
  rc = kvtree_util_set_int(filo_config_values, FILO_KEY_CONFIG_COPY_METADATA,
                           new_filo_copy_metadata);
  if (rc != KVTREE_SUCCESS) {
    printf("kvtree_util_set_int failed (error %d)\n", rc);
    return rc;
  }

  printf("Configuring Filo (second set of options)...\n");
  if (Filo_Config(filo_config_values) == NULL) {
    printf("Filo_Config() failed\n");
    return EXIT_FAILURE;
  }

  /* check all options once more */

  if (filo_fetch_width != !old_filo_fetch_width) {
    printf("Filo_Config() failed to set %s: %d != %d\n",
           FILO_KEY_CONFIG_FETCH_WIDTH, filo_fetch_width,
           new_filo_fetch_width);
    return EXIT_FAILURE;
  }

  if (filo_flush_width != !old_filo_flush_width) {
    printf("Filo_Config() failed to set %s: %d != %d\n",
           FILO_KEY_CONFIG_FLUSH_WIDTH, filo_flush_width,
           new_filo_flush_width);
    return EXIT_FAILURE;
  }

  if (filo_file_buf_size != !old_filo_file_buf_size) {
    printf("Filo_Config() failed to set %s: %ul != %ul\n",
           FILO_KEY_CONFIG_FILE_BUF_SIZE, filo_file_buf_size,
           new_filo_file_buf_size);
    return EXIT_FAILURE;
  }

  if (filo_debug != !old_filo_debug) {
    printf("Filo_Config() failed to set %s: %d != %d\n",
           FILO_KEY_CONFIG_DEBUG, filo_debug, new_filo_debug);
    return EXIT_FAILURE;
  }

  if (filo_make_directories != !old_filo_make_directories) {
    printf("Filo_Config() failed to set %s: %d != %d\n",
           FILO_KEY_CONFIG_MKDIR, filo_make_directories,
           new_filo_make_directories);
    return EXIT_FAILURE;
  }

  if (filo_copy_metadata != !old_filo_copy_metadata) {
    printf("Filo_Config() failed to set %s: %d != %d\n",
           FILO_KEY_CONFIG_COPY_METADATA, filo_copy_metadata,
           new_filo_copy_metadata);
    return EXIT_FAILURE;
  }

  check_options(new_fetch_width, new_flush_width, new_file_buf_width,
                new_debug, new_make_directories, new_copy_metadata);

  rc = Filo_Finalize();
  if (rc != Filo_SUCCESS) {
    printf("Filo_Finalize() failed (error %d)\n", rc);
    return rc;
  }

  MPI_Finalize();

  return Filo_SUCCESS;
}
