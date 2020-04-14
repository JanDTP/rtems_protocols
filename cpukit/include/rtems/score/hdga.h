/*
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_HDGA_H
#define _RTEMS_SCORE_HDGA_H

#include <rtems/score/cpuopts.h>
#include <rtems/score/ticket.h>

#if defined(RTEMS_SMP)

#include <rtems/score/threadq.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief HDGA control block.
 */
typedef struct {
  /**
   * @brief The thread queue to manage ownership and waiting threads.
   */
  Thread_queue_Control Wait_queue;

  /**
   *ticket array, where we store our tickets and the order
   */
  Ticket_Control *ticket_order;

  /**
   * size of ticket_order
   */
  int order_size;

  /**
   * current_position of array
   */
  int current_position;

} HDGA_Control;

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_HDGA_H */
