/*
 * Copyright (c) 2018 Malte Muench.  All rights reserved.
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution or at
 * http://www.rtems.org/license/LICENSE.
 */

#ifndef _RTEMS_SCORE_FMLPSIMPL_H
#define _RTEMS_SCORE_FMLPSIMPL_H


#include <rtems/score/fmlps.h>
#if defined(RTEMS_SMP)

#include <rtems/score/assert.h>
#include <rtems/score/status.h>
#include <rtems/score/threadqimpl.h>
#include <rtems/score/watchdogimpl.h>
#include <rtems/score/wkspace.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup ScoreFMLPS
 *
 * @{
 */

#define FMLPS_TQ_OPERATIONS &_Thread_queue_Operations_FIFO

/**
 * @brief Acquires critical according to FMLP-S.
 *
 * @param fmlps The FMLP-L control for the operation.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _FMLPS_Acquire_critical(
  FMLPS_Control        *fmlps,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Acquire_critical( &fmlps->Wait_queue, queue_context );
}

/**
 * @brief Releases according to FMLP-S.
 *
 * @param fmlps The FMLP-S control for the operation.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _FMLPS_Release(
  FMLPS_Control        *fmlps,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Release( &fmlps->Wait_queue, queue_context );
}

/**
 * @brief Gets owner of the FMLP-S control.
 *
 * @param fmlps The FMLP-S control to get the owner from.
 *
 * @return The owner of the FMLP-S control.
 */
RTEMS_INLINE_ROUTINE Thread_Control *_FMLPS_Get_owner(
  const FMLPS_Control *fmlps
)
{
  return fmlps->Wait_queue.Queue.owner;
}

/**
 * @brief Sets owner of the FMLP-S control.
 *
 * @param[out] fmlps The FMLP-S control to set the owner of.
 * @param owner The desired new owner for fmlps
 */
RTEMS_INLINE_ROUTINE void _FMLPS_Set_owner(
  FMLPS_Control  *fmlps,
  Thread_Control *owner
)
{
  fmlps->Wait_queue.Queue.owner = owner;
}

/**
 * @brief Gets priority of the FMLP-S control.
 *
 * @param fmlps The fmlps to get the priority from.
 * @param scheduler The corresponding scheduler.
 *
 * @return The priority of the FMLP-S control.
 */
RTEMS_INLINE_ROUTINE Priority_Control _FMLPS_Get_priority(
  const FMLPS_Control     *fmlps,
  const Scheduler_Control *scheduler
)
{
  uint32_t scheduler_index;

  scheduler_index = _Scheduler_Get_index( scheduler );
  return fmlps->ceiling_priorities[ scheduler_index ];
}

/**
 * @brief Sets priority of the FMLP-S control
 *
 * @param[out] fmlps The FMLP-S control to set the priority of.
 * @param scheduler The corresponding scheduler.
 * @param new_priority The new priority for the FMLP-S control
 */
RTEMS_INLINE_ROUTINE void _FMLPS_Set_priority(
  FMLPS_Control           *fmlps,
  const Scheduler_Control *scheduler,
  Priority_Control         new_priority
)
{
  // do nothing, the priority is always 1
}

/**
 * @brief Adds the priority to the given thread.
 *
 * @param fmlps The FMLP-S control for the operation.
 * @param[in, out] thread The thread to add the priority node to.
 * @param[out] priority_node The priority node to initialize and add to
 *      the thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the thread
 *      exceeds the ceiling priority.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Raise_priority(
  FMLPS_Control        *fmlps,
  Thread_Control       *thread,
  Priority_Node        *priority_node,
  Thread_queue_Context *queue_context
)
{
  Status_Control           status;
  ISR_lock_Context         lock_context;
  const Scheduler_Control *scheduler;
  Priority_Control         ceiling_priority;
  Scheduler_Node          *scheduler_node;

  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( thread, &lock_context );

  scheduler = _Thread_Scheduler_get_home( thread );
  scheduler_node = _Thread_Scheduler_get_home_node( thread );
  ceiling_priority = _FMLPS_Get_priority( fmlps, scheduler );

  if (
    ceiling_priority
      <= _Priority_Get_priority( &scheduler_node->Wait.Priority )
  ) {
    _Priority_Node_initialize( priority_node, ceiling_priority );
    _Thread_Priority_add( thread, priority_node, queue_context );
    status = STATUS_SUCCESSFUL;
  } else {
    status = STATUS_MUTEX_CEILING_VIOLATED;
  }

  _Thread_Wait_release_default_critical( thread, &lock_context );
  return status;
}

/**
 * @brief Removes the priority from the given thread.
 *
 * @param[in, out] The thread to remove the priority from.
 * @param priority_node The priority node to remove from the thread
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _FMLPS_Remove_priority(
  Thread_Control       *thread,
  Priority_Node        *priority_node,
  Thread_queue_Context *queue_context
)
{
  ISR_lock_Context lock_context;

  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( thread, &lock_context );
  _Thread_Priority_remove( thread, priority_node, queue_context );
  _Thread_Wait_release_default_critical( thread, &lock_context );
}

/**
 * @brief Replaces the given priority node with the ceiling priority of
 *      the FMLP-S control.
 *
 * @param fmlps The fmlps control for the operation.
 * @param[out] thread The thread to replace the priorities.
 * @param ceiling_priority The node to be replaced.
 */
RTEMS_INLINE_ROUTINE void _FMLPS_Replace_priority(
  FMLPS_Control  *fmlps,
  Thread_Control *thread,
  Priority_Node  *ceiling_priority
)
{
  ISR_lock_Context lock_context;

  _Thread_Wait_acquire_default( thread, &lock_context );
  _Thread_Priority_replace( 
    thread,
    ceiling_priority,
    &fmlps->Ceiling_priority
  );
  _Thread_Wait_release_default( thread, &lock_context );
}

/**
 * @brief Claims ownership of the FMLP-S control.
 *
 * @param fmlps The FMLP-S control to claim the ownership of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the executing
 *      thread exceeds the ceiling priority.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Claim_ownership(
  FMLPS_Control        *fmlps,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Status_Control   status;
  Per_CPU_Control *cpu_self;

  status = _FMLPS_Raise_priority(
    fmlps,
    executing,
    &fmlps->Ceiling_priority,
    queue_context
  );

  if ( status != STATUS_SUCCESSFUL ) {
    _FMLPS_Release( fmlps, queue_context );
    return status;
  }

  _FMLPS_Set_owner( fmlps, executing );
  cpu_self = _Thread_queue_Dispatch_disable( queue_context );
  _FMLPS_Release( fmlps, queue_context );
  _Thread_Priority_and_sticky_update( executing, 1 );
  _Thread_Dispatch_enable( cpu_self );
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Initializes a FMLP-S control.
 *
 * @param[out] fmlps The FMLP-S control that is initialized.
 * @param scheduler The scheduler for the operation.
 * @param ceiling_priority
 * @param executing The currently executing thread.  Ignored in this method.
 * @param initially_locked Indicates whether the FMLP-S control shall be initally
 *      locked. If it is initially locked, this method returns STATUS_INVALID_NUMBER.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_INVALID_NUMBER The FMLP-S control is initially locked.
 * @retval STATUS_NO_MEMORY There is not enough memory to allocate.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Initialize(
  FMLPS_Control           *fmlps,
  const Scheduler_Control *scheduler,
  Priority_Control         ceiling_priority,
  Thread_Control          *executing,
  bool                     initially_locked
)
{
  uint32_t scheduler_count = _Scheduler_Count;
  uint32_t i;
  // this priority should emulate the "non-preemptability" of FMLP-S
  ceiling_priority = 1;

  if ( initially_locked ) {
    return STATUS_INVALID_NUMBER;
  }

  fmlps->ceiling_priorities = _Workspace_Allocate(
    sizeof( *fmlps->ceiling_priorities ) * scheduler_count
  );
  if ( fmlps->ceiling_priorities == NULL ) {
    return STATUS_NO_MEMORY;
  }

  for ( i = 0 ; i < scheduler_count ; ++i ) {
    const Scheduler_Control *scheduler_of_index;

    scheduler_of_index = &_Scheduler_Table[ i ];

    if ( scheduler != scheduler_of_index ) {
      fmlps->ceiling_priorities[ i ] =
        _Scheduler_Map_priority( scheduler_of_index, 0 );
    } else {
      fmlps->ceiling_priorities[ i ] = ceiling_priority;
    }
  }

  _Thread_queue_Object_initialize( &fmlps->Wait_queue );
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Waits for the ownership of the FMLP-S control.
 *
 * @param[in, out] fmlps The FMLP-S control to get the ownership of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context the thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the
 *      currently executing thread exceeds the ceiling priority.
 * @retval STATUS_DEADLOCK A deadlock occured.
 * @retval STATUS_TIMEOUT A timeout occured.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Wait_for_ownership(
  FMLPS_Control        *fmlps,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Status_Control status;
  Priority_Node  ceiling_priority;

  status = _FMLPS_Raise_priority(
    fmlps,
    executing,
    &ceiling_priority,
    queue_context
  );

  if ( status != STATUS_SUCCESSFUL ) {
    _FMLPS_Release( fmlps, queue_context );
    return status;
  }

  _Thread_queue_Context_set_deadlock_callout(
    queue_context,
    _Thread_queue_Deadlock_status
  );
  status = _Thread_queue_Enqueue_sticky(
    &fmlps->Wait_queue.Queue,
    FMLPS_TQ_OPERATIONS,
    executing,
    queue_context
  );

  if ( status == STATUS_SUCCESSFUL ) {
    _FMLPS_Replace_priority( fmlps, executing, &ceiling_priority );
  } else {
    Thread_queue_Context  queue_context;
    Per_CPU_Control      *cpu_self;
    int                   sticky_level_change;

    if ( status != STATUS_DEADLOCK ) {
      sticky_level_change = -1;
    } else {
      sticky_level_change = 0;
    }

    _ISR_lock_ISR_disable( &queue_context.Lock_context.Lock_context );
    _FMLPS_Remove_priority( executing, &ceiling_priority, &queue_context );
    cpu_self = _Thread_Dispatch_disable_critical(
      &queue_context.Lock_context.Lock_context
    );
    _ISR_lock_ISR_enable( &queue_context.Lock_context.Lock_context );
    _Thread_Priority_and_sticky_update( executing, sticky_level_change );
    _Thread_Dispatch_enable( cpu_self );
  }

  return STATUS_SUCCESSFUL;
}

/**
 * @brief Seizes the FMLP-S control.
 *
 * @param[in, out] fmlps The FMLP-S control to seize the control of.
 * @param[in, out] executing The currently executing thread.
 * @param wait Indicates whether the calling thread is willing to wait.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the executing
 *      thread exceeds the ceiling priority.
 * @retval STATUS_UNAVAILABLE The executing thread is already the owner of
 *      the FMLP-S control.  Seizing it is not possible.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Seize(
  FMLPS_Control        *fmlps,
  Thread_Control       *executing,
  bool                  wait,
  Thread_queue_Context *queue_context
)
{
  Status_Control  status;
  Thread_Control *owner;

  _FMLPS_Acquire_critical( fmlps, queue_context );
  owner = _FMLPS_Get_owner( fmlps );

  if ( owner == NULL ) {
    status = _FMLPS_Claim_ownership( fmlps, executing, queue_context );
  } else if ( owner == executing ) {
    _FMLPS_Release( fmlps, queue_context );
    status = STATUS_UNAVAILABLE;
  } else if ( wait ) {
    status = _FMLPS_Wait_for_ownership( fmlps, executing, queue_context );
  } else {
    _FMLPS_Release( fmlps, queue_context );
    status = STATUS_UNAVAILABLE;
  }

  return status;
}

/**
 * @brief Surrenders the FMLP-S control.
 *
 * @param[in, out] fmlps The FMLP-S control to surrender the control of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_NOT_OWNER The executing thread does not own the FMLP-S control.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Surrender(
  FMLPS_Control        *fmlps,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Thread_queue_Heads *heads;
  if ( _FMLPS_Get_owner( fmlps ) != executing ) {
    _ISR_lock_ISR_enable( &queue_context->Lock_context.Lock_context );
    return STATUS_NOT_OWNER;
  }

  _FMLPS_Acquire_critical( fmlps, queue_context );

  _FMLPS_Set_owner( fmlps, NULL );
  _FMLPS_Remove_priority( executing, &fmlps->Ceiling_priority, queue_context );

  heads = fmlps->Wait_queue.Queue.heads;

  if ( heads == NULL ) {
    Per_CPU_Control *cpu_self;

    cpu_self = _Thread_Dispatch_disable_critical(
      &queue_context->Lock_context.Lock_context
    );
    _FMLPS_Release( fmlps, queue_context );
    _Thread_Priority_and_sticky_update( executing, -1 );
    _Thread_Dispatch_enable( cpu_self );
    return RTEMS_SUCCESSFUL;
  }

  _Thread_queue_Surrender_sticky(
    &fmlps->Wait_queue.Queue,
    heads,
    executing,
    queue_context,
    FMLPS_TQ_OPERATIONS
  );
  return RTEMS_SUCCESSFUL;
}

/**
 * @brief Checks if the FMLP-S control can be destroyed.
 *
 * @param fmlps The FMLP-S control for the operation.
 *
 * @retval STATUS_SUCCESSFUL The FMLP-S is currently not used
 *      and can be destroyed.
 * @retval STATUS_RESOURCE_IN_USE The FMLP-S control is in use,
 *      it cannot be destroyed.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Can_destroy( FMLPS_Control *fmlps )
{
  if ( _FMLPS_Get_owner( fmlps ) != NULL ) {
    return STATUS_RESOURCE_IN_USE;
  }

  return STATUS_SUCCESSFUL;
}

/**
 * @brief Destroys the FMLP-S control
 *
 * @param[in, out] The fmlps that is about to be destroyed.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _FMLPS_Destroy(
  FMLPS_Control        *fmlps,
  Thread_queue_Context *queue_context
)
{
  _FMLPS_Release( fmlps, queue_context );
  _Thread_queue_Destroy( &fmlps->Wait_queue );
  _Workspace_Free( fmlps->ceiling_priorities );
}

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_FMLPSIMPL_H */
