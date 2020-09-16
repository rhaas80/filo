#ifndef FILO_INTERNAL_H
#define FILO_INTERNAL_H

/** \defgroup filo FILO
 *  \brief Internal variables for FILO */

/** \file filo_internal.h
 *  \ingroup filo
 *  \brief Internal variables for FILO */

/* enable C++ codes to include this header directly */
#ifdef __cplusplus
extern "C" {
#endif

extern int filo_fetch_width; /**< number of processes to read files simultaneously */
extern int filo_flush_width; /**< number of processes to write files simultaneously*/

/* enable C++ codes to include this header directly */
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* FILO_INTERNAL_H */
