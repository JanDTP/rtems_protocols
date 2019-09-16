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

// do nothing instead of helping.
#define FMLPS_TQ_OPERATIONS &_Thread_queue_Operations_FIFO

RTEMS_INLINE_ROUTINE uint64_t _FMLPS_Get_Nanoseconds( void )
{
  Timestamp_Control  snapshot_as_timestamp;
  _TOD_Get_zero_based_uptime(&snapshot_as_timestamp);
  return _Timestamp_Get_as_nanoseconds(&snapshot_as_timestamp);
}


RTEMS_INLINE_ROUTINE void _FMLPS_Acquire_critical(
  FMLPS_Control        *fmlps,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Acquire_critical( &fmlps->Wait_queue, queue_context );
}

RTEMS_INLINE_ROUTINE void _FMLPS_Release(
  FMLPS_Control        *fmlps,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Release( &fmlps->Wait_queue, queue_context );
}

RTEMS_INLINE_ROUTINE Thread_Control *_FMLPS_Get_owner(
  const FMLPS_Control *fmlps
)
{
  return fmlps->Wait_queue.Queue.owner;
}

RTEMS_INLINE_ROUTINE void _FMLPS_Set_owner(
  FMLPS_Control  *fmlps,
  Thread_Control *owner
)
{
  fmlps->Wait_queue.Queue.owner = owner;
}

RTEMS_INLINE_ROUTINE Priority_Control _FMLPS_Get_priority(
  const FMLPS_Control     *fmlps,
  const Scheduler_Control *scheduler
)
{
  uint32_t scheduler_index;

  scheduler_index = _Scheduler_Get_index( scheduler );
  return fmlps->ceiling_priorities[ scheduler_index ];
}

RTEMS_INLINE_ROUTINE void _FMLPS_Set_priority(
  FMLPS_Control           *fmlps,
  const Scheduler_Control *scheduler,
  Priority_Control         new_priority
)
{
  // do nothing, the priority is always 1
}

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

RTEMS_INLINE_ROUTINE void _FMLPS_Replace_priority(
  FMLPS_Control   *fmlps,
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

RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Claim_ownership(
  FMLPS_Control        *fmlps,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Status_Control   status;
  Per_CPU_Control *cpu_self;
  uint64_t start, end;
  start = _FMLPS_Get_Nanoseconds();

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
  end = _FMLPS_Get_Nanoseconds();
  //return STATUS_SUCCESSFUL;
  return (end-start);
}

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

  // this priority should emulate the "non-preemptability" of FMLPS
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

RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Wait_for_ownership(
  FMLPS_Control        *fmlps,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Status_Control status;
  Priority_Node  ceiling_priority;
  uint64_t start, end, preq, postq;
  start = _FMLPS_Get_Nanoseconds();

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
  preq = _FMLPS_Get_Nanoseconds();
  status = _Thread_queue_Enqueue_sticky(
    &fmlps->Wait_queue.Queue,
    FMLPS_TQ_OPERATIONS,
    executing,
    queue_context
  );
  postq = _FMLPS_Get_Nanoseconds();

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

  end = _FMLPS_Get_Nanoseconds();
  //return STATUS_SUCCESSFUL;
  return 1;
}

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
 // this point will be executed by exactly one or no task
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

RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Surrender(
  FMLPS_Control        *fmlps,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Thread_queue_Heads *heads;
  uint64_t start, end;
  start = _FMLPS_Get_Nanoseconds();

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
    end = _FMLPS_Get_Nanoseconds();
    _Thread_Dispatch_enable( cpu_self );
    return (end-start);
  }

  end = _Thread_queue_Surrender_sticky(
    &fmlps->Wait_queue.Queue,
    heads,
    executing,
    queue_context,
    FMLPS_TQ_OPERATIONS
  );
  return (end-start);
}

RTEMS_INLINE_ROUTINE Status_Control _FMLPS_Can_destroy( FMLPS_Control *fmlps )
{
  if ( _FMLPS_Get_owner( fmlps ) != NULL ) {
    return STATUS_RESOURCE_IN_USE;
  }

  return STATUS_SUCCESSFUL;
}

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
