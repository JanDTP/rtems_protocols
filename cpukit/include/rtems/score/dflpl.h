/*
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_DFLPL_H
#define _RTEMS_SCORE_DFLPL_H

#include <rtems/score/cpuopts.h>

#if defined(RTEMS_SMP)

#include <rtems/score/threadq.h>
#include <rtems/score/percpu.h>
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief DFLPL control block.
 */
typedef struct {
  // our thread queue
  Thread_queue_Control Wait_queue;

  // the priority node, which is assigned to the priority owner
  Priority_Node root_node;

  // our array for keeping the priorities of all tasks
  Priority_Control *priority_array;

  // current first free position
  int first_free_slot;

  //Pointer to the synchronization cpu control structure
  Per_CPU_Control *pu;
} DFLPL_Control;

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_DFLPL_H */
