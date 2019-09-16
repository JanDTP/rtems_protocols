/*
 * Copyright (c) 2014, 2016 embedded brains GmbH.  All rights reserved.
 *
 *  embedded brains GmbH
 *  Dornierstr. 4
 *  82178 Puchheim
 *  Germany
 *  <rtems@embedded-brains.de>
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_FMLPL_H
#define _RTEMS_SCORE_FMLPL_H

#include <rtems/score/cpuopts.h>

#if defined(RTEMS_SMP)

#include <rtems/score/threadq.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @defgroup ScoreFMLPL Multiprocessor Resource Sharing Protocol Handler (This guy didn't even edit this LOL)
 *
 * @ingroup Score
 *
 *
 * @{
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

} FMLPL_Control;

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_FMLPL_H */
