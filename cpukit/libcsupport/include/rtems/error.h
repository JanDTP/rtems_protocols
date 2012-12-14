/**
 * @file rtems/error.h
 *
 * Defines and externs for rtems error reporting
 * 
 * Currently just used by RTEMS monitor.
 * 
 * These routines provide general purpose error reporting.
 * rtems_error reports an error to stderr and allows use of
 * printf style formatting.  A newline is appended to all messages.
 *
 * error_flag can be specified as any of the following:
 *
 *  RTEMS_ERROR_ERRNO       -- include errno text in output
 *  RTEMS_ERROR_PANIC       -- halts local system after output
 *  RTEMS_ERROR_ABORT       -- abort after output
 *
 * It can also include a rtems_status value which can be OR'd
 * with the above flags. *
 *
 * EXAMPLE
 *  #include <rtems.h>
 *  #include <rtems/error.h>
 *  rtems_error(0, "stray interrupt %d", intr);
 *
 * EXAMPLE
 *        if ((status = rtems_task_create(...)) != RTEMS_SUCCCESSFUL)
 *        {
 *            rtems_error(status | RTEMS_ERROR_ABORT,
 *                        "could not create task");
 *        }
 *
 * EXAMPLE
 *        if ((fd = open(pathname, O_RDNLY)) < 0)
 *        {
 *            rtems_error(RTEMS_ERROR_ERRNO, "open of '%s' failed", pathname);
 *            goto failed;
 *        }
 */


#ifndef _RTEMS_RTEMS_ERROR_H
#define _RTEMS_RTEMS_ERROR_H

#include <rtems/rtems/status.h>
#include <rtems/score/interr.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  @defgroup ErrorPanicSupport Error And Panic Support
 *
 *  @ingroup libcsupport
 * 
 *  @brief Defines and externs for rtems error reporting
 * 
 */

typedef Internal_errors_t rtems_error_code_t;

/*
 * rtems_error() and rtems_panic() support
 */

#if 0
/* not 16bit-int host clean */
#define RTEMS_ERROR_ERRNO  (1<<((sizeof(rtems_error_code_t) * CHAR_BIT) - 2)) /* hi bit; use 'errno' */
#define RTEMS_ERROR_PANIC  (RTEMS_ERROR_ERRNO / 2)       /* err fatal; no return */
#define RTEMS_ERROR_ABORT  (RTEMS_ERROR_ERRNO / 4)       /* err is fatal; panic */
#else
#define RTEMS_ERROR_ERRNO  (0x40000000) /* hi bit; use 'errno' */
#define RTEMS_ERROR_PANIC  (0x20000000) /* err fatal; no return */
#define RTEMS_ERROR_ABORT  (0x10000000) /* err is fatal; panic */
#endif

#define RTEMS_ERROR_MASK \
  (RTEMS_ERROR_ERRNO | RTEMS_ERROR_ABORT | RTEMS_ERROR_PANIC) /* all */

const char *rtems_status_text(rtems_status_code sc);

/**
 *  @brief Report an Error
 * 
 *  @param[in] error_code can be specified as any of the following:
 *  RTEMS_ERROR_ERRNO       -- include errno text in output
 *  RTEMS_ERROR_PANIC       -- halts local system after output
 *  RTEMS_ERROR_ABORT       -- abort after output
 * 
 *  @param[in] printf_format is a normal printf(3) format string, 
 *  with its concommitant arguments
 *
 *  @return the number of characters written.
 */
int   rtems_error(
  rtems_error_code_t error_code,
  const char *printf_format,
  ...
);

/**
 * rtems_panic is shorthand for rtems_error(RTEMS_ERROR_PANIC, ...)
 */
void rtems_panic(
  const char *printf_format,
  ...
) RTEMS_COMPILER_NO_RETURN_ATTRIBUTE;

extern int rtems_panic_in_progress;

#ifdef __cplusplus
}
#endif


#endif
/* end of include file */
