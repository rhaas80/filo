#include <stdio.h>
#include <stdlib.h>
#include "filo.h"
#include "filo_internal.h"

#include "kvtree.h"
#include "kvtree_util.h"

int
main(int argc, char *argv[]) {
    int rc;
    char *conf_file = NULL;
    kvtree* filo_config_values = kvtree_new();

    /* TODO: check options that are just passed on to AXL */
    int old_filo_fetch_width = filo_fetch_width;
    int old_filo_flush_width = filo_flush_width;

    MPI_Init(&argc, &argv);

    rc = Filo_Init(conf_file);
    if (rc != Filo_SUCCESS) {
        printf("Filo_Init() failed (error %d)\n", rc);
        return rc;
    }

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
               Filo_KEY_CONFIG_FETCH_WIDTH, filo_fetch_width, old_filo_fetch_width+1);
        return EXIT_FAILURE;
    }

    /* set remainder of options */
    kvtree_delete(&filo_config_values);
    filo_config_values = kvtree_new();

    rc = kvtree_util_set_int(filo_config_values, FILO_KEY_CONFIG_FLUSH_WIDTH,
                             old_filo_flush_width + 1);
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
               Filo_KEY_CONFIG_FETCH_WIDTH, filo_fetch_width, old_filo_fetch_width+1);
        return EXIT_FAILURE;
    }

    if (filo_flush_width != !old_filo_flush_width) {
        printf("Filo_Config() failed to set %s: %d != %d\n",
               Filo_KEY_CONFIG_FLUSH_WIDTH, filo_flush_width, old_filo_flush_width+1);
        return EXIT_FAILURE;
    }

    rc = Filo_Finalize();
    if (rc != Filo_SUCCESS) {
        printf("Filo_Finalize() failed (error %d)\n", rc);
        return rc;
    }

    MPI_Finalize();

    return Filo_SUCCESS;
}
