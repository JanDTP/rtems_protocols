/*
 * Copyright (c) 2018 Malte Muench.  All rights reserved.
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_FMLPS_H
#define _RTEMS_SCORE_FMLPS_H

#include <rtems/score/cpuopts.h>

#if defined(RTEMS_SMP)

#include <rtems/score/threadq.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief FMLPS control block.
 */
typedef struct {
  /**
   * @brief The thread queue to manage ownership and waiting threads.
   */
  Thread_queue_Control Wait_queue;

  /**
   * @brief The ceiling priority used by the owner thread.
   */
  Priority_Node Ceiling_priority;

  /**
   * @brief One ceiling priority per scheduler instance.
   */
  Priority_Control *ceiling_priorities;
} FMLPS_Control;

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_FMLPS_H */
