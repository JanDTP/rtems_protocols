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

#ifndef _RTEMS_SCORE_FMLPLIMPL_H
#define _RTEMS_SCORE_FMLPLIMPL_H

#include <rtems/score/fmlpl.h>

#if defined(RTEMS_SMP)

#include <rtems/score/assert.h>
#include <rtems/score/status.h>
#include <rtems/score/threadqimpl.h>
#include <rtems/rtems/tasksimpl.h>
#include <rtems/score/watchdogimpl.h>
#include <rtems/score/wkspace.h>
#include <rtems/score/rbtree.h>
#include <rtems/bspIo.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup ScoreFMLPL
 *
 * @{
 */
#define FMLPL_TQ_OPERATIONS &_Thread_queue_Operations_FIFO

RTEMS_INLINE_ROUTINE uint64_t _FMLPL_Get_Nanoseconds( void )
{
  Timestamp_Control  snapshot_as_timestamp;
  _TOD_Get_zero_based_uptime(&snapshot_as_timestamp);
  return _Timestamp_Get_as_nanoseconds(&snapshot_as_timestamp);
}

RTEMS_INLINE_ROUTINE void _FMLPL_Acquire_critical(
  FMLPL_Control        *fmlpl,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Acquire_critical( &fmlpl->Wait_queue, queue_context );
}

RTEMS_INLINE_ROUTINE void _FMLPL_Release(
  FMLPL_Control        *fmlpl,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Release( &fmlpl->Wait_queue, queue_context );
}

RTEMS_INLINE_ROUTINE Thread_Control *_FMLPL_Get_owner(
  const FMLPL_Control *fmlpl
)
{
  return fmlpl->Wait_queue.Queue.owner;
}

RTEMS_INLINE_ROUTINE void _FMLPL_Set_owner(
  FMLPL_Control  *fmlpl,
  Thread_Control *owner
)
{
  fmlpl->Wait_queue.Queue.owner = owner;
}

RTEMS_INLINE_ROUTINE Priority_Control _FMLPL_Get_priority(
  const Thread_Control *the_thread
)
{
  const Scheduler_Node    *scheduler_node;
  const Scheduler_Control *scheduler;
  Priority_Control         core_priority;

  scheduler = _Thread_Scheduler_get_home( the_thread );
  scheduler_node = _Thread_Scheduler_get_home_node( the_thread );

  core_priority = _Priority_Get_priority( &scheduler_node->Wait.Priority );
  return _RTEMS_Priority_From_core( scheduler, core_priority );
}
  
RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Change_Owner_Priority(
  Priority_Control      new_prio,
  FMLPL_Control        *fmlpl,
  Thread_queue_Context *queue_context
)
{
  Thread_Control   *owner;
  ISR_lock_Context  lock_context;
  Priority_Node    *priority_node;

  priority_node = &( fmlpl->root_node );
  owner = _FMLPL_Get_owner( fmlpl );
  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( owner, &lock_context );
  _Thread_Priority_remove( owner, priority_node, queue_context );
  _Priority_Node_initialize( priority_node, SCHEDULER_PRIORITY_MAP( new_prio ) );
  _Thread_Priority_add( owner, priority_node, queue_context );
  _Thread_Wait_release_default_critical( owner, &lock_context );
  return RTEMS_SUCCESSFUL;
}
  
RTEMS_INLINE_ROUTINE Priority_Control _FMLPL_Get_Min_Priority(
  FMLPL_Control *fmlpl
)
{
  int i, fs;
  Priority_Control min_prio;
  min_prio = PRIORITY_DEFAULT_MAXIMUM;
  fs = fmlpl->first_free_slot;
    
  for ( i = 0; i < fs; i++ ) {
    if ( fmlpl->priority_array[i] < min_prio ) {
      min_prio = fmlpl->priority_array[i];
      }
    }
      
    return min_prio;
}

RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Print_Queue(
  FMLPL_Control *fmlpl
)
{
  int i;

  for ( i = 0; i < 16; i++ ) {
    if ( fmlpl->priority_array[i] == 0 ) {
    } else {

    }
  }

  for( i = 0; i < fmlpl->first_free_slot; i++ ) {
  }

  for( i = 0; i < fmlpl->first_free_slot; i++ ) {
  }

  for( i = 0; i < fmlpl->first_free_slot; i++ ) {
  }
}

/*
 * Inserts the priority of executing into the waiting array, returns 0 if
 * successful 1 otherwise. Causes of error could be:
 *
 * - a full queue
 * - a mismatch between free slot and free slot number
 * - a free slot number not between 0 and 15.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Insert(
  FMLPL_Control  *fmlpl,
  Thread_Control *executing
)
{
  if( fmlpl->first_free_slot < 0 ) {
    return 1;
  }

  if( fmlpl->first_free_slot > 15 ) {
    return 1;
  }

  if( fmlpl->priority_array[fmlpl->first_free_slot] != 0 ) {
    return 1;
  }
  fmlpl->priority_array[fmlpl->first_free_slot] = _FMLPL_Get_priority( executing );
  if(fmlpl->first_free_slot==15) {
  }
  fmlpl->first_free_slot++;
  return 0;
}

RTEMS_INLINE_ROUTINE Status_Control _FMLPL_remove(
  FMLPL_Control *fmlpl
)
{
  int              i;
  int              fs;
  Priority_Control head;

  if( fmlpl->first_free_slot < 0 ) {
    return 1;
  }

  if( fmlpl->first_free_slot > 15 ) {
    return 1;
  }

  if( fmlpl->priority_array[fmlpl->first_free_slot] != 0 ) {
    return 1;
  }

  head = fmlpl->priority_array[0];
  fs = fmlpl->first_free_slot;
  for( i = 0; i < fs; i++ ) {
    fmlpl->priority_array[i] = fmlpl->priority_array[i+1];
  }
  fmlpl->first_free_slot--;
  fs = fmlpl->first_free_slot;
  fmlpl->priority_array[fs] = 0;

    return 0;
}

RTEMS_INLINE_ROUTINE Status_Control _FMLPL_add(
  FMLPL_Control        *fmlpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Priority_Control min;
  Priority_Control newmin;

  min = _FMLPL_Get_Min_Priority( fmlpl );
  _FMLPL_Insert( fmlpl, executing );

  newmin = _FMLPL_Get_Min_Priority( fmlpl );
  if ( newmin < min ) {
    _FMLPL_Change_Owner_Priority( newmin, fmlpl, queue_context );
  }
  return RTEMS_SUCCESSFUL;
}


RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Claim_ownership(
  FMLPL_Control        *fmlpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{

  ISR_lock_Context  lock_context;
  Priority_Node    *priority_node;
  uint64_t start, end;

  start = _FMLPL_Get_Nanoseconds();
  _FMLPL_Set_owner( fmlpl, executing );
  priority_node = &( fmlpl->root_node );
  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( executing, &lock_context );
  _Priority_Node_initialize(
    priority_node,
    SCHEDULER_PRIORITY_MAP( _FMLPL_Get_Min_Priority( fmlpl ))
  );
  if( _FMLPL_Get_Min_Priority != 255 ) { //???
    _Thread_Priority_add( executing, priority_node, queue_context );
  }
  _Thread_Wait_release_default_critical( executing, &lock_context );

  end = _FMLPL_Get_Nanoseconds();
  _FMLPL_Release( fmlpl, queue_context );

  return end-start;
}

RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Initialize(
  FMLPL_Control           *fmlpl,
  const Scheduler_Control *scheduler,
  Priority_Control         ceiling_priority,
  Thread_Control          *executing,
  bool                     initially_locked
)
{
  Priority_Node *priority_node;
  int            i;

  if ( initially_locked ) {
    return STATUS_INVALID_NUMBER;
  }
  _Thread_queue_Object_initialize( &fmlpl->Wait_queue );

  fmlpl->priority_array = _Workspace_Allocate(
      sizeof( *fmlpl->priority_array ) * 16
  );

  for ( i = 0; i < 16; i++ ) {
    fmlpl->priority_array[i] = 0;
  }
  fmlpl->first_free_slot = 0;
  priority_node = &( fmlpl->root_node );
  return STATUS_SUCCESSFUL;
}

RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Wait_for_ownership(
  FMLPL_Control        *fmlpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  uint64_t start, end, preq, postq;
  start = _FMLPL_Get_Nanoseconds();
  _FMLPL_add(fmlpl, executing, queue_context);

  _Thread_queue_Context_set_thread_state(
    queue_context,
    STATES_WAITING_FOR_MUTEX
  );
  _Thread_queue_Context_set_enqueue_do_nothing_extra( queue_context );
  _Thread_queue_Context_set_deadlock_callout(
    queue_context,
    _Thread_queue_Deadlock_status
  );
  preq = _FMLPL_Get_Nanoseconds();
  _Thread_queue_Enqueue(
    &fmlpl->Wait_queue.Queue,
    FMLPL_TQ_OPERATIONS,
    executing,
    queue_context
  );
  postq = _FMLPL_Get_Nanoseconds();
  _FMLPL_Acquire_critical( fmlpl, queue_context );
  Priority_Control new_high_prio;
  _FMLPL_remove( fmlpl );
  new_high_prio = _FMLPL_Get_Min_Priority( fmlpl );
  ISR_lock_Context         lock_context;
  Priority_Node            *priority_node;

  priority_node = &( fmlpl->root_node );
  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( executing, &lock_context );
  _Priority_Node_initialize(
    priority_node,
    SCHEDULER_PRIORITY_MAP( new_high_prio )
  );
  _Thread_Priority_add( executing, priority_node, queue_context );
  _Thread_Wait_release_default_critical( executing, &lock_context );
  _FMLPL_Release( fmlpl, queue_context );
  end = _FMLPL_Get_Nanoseconds();
  //return (end-start-(postq-preq));
  return 1;
}


RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Seize(
  FMLPL_Control        *fmlpl,
  Thread_Control       *executing,
  bool                  wait,
  Thread_queue_Context *queue_context
)
{
  Status_Control  status;
  Thread_Control *owner;

  _FMLPL_Acquire_critical( fmlpl, queue_context );

  owner = _FMLPL_Get_owner( fmlpl );

  if ( owner == NULL ) {
    status = _FMLPL_Claim_ownership( fmlpl, executing, queue_context );
  } else if ( owner == executing ) {
    _FMLPL_Release( fmlpl, queue_context );
    status = STATUS_UNAVAILABLE;
  } else if ( wait ) {
    status = _FMLPL_Wait_for_ownership( fmlpl, executing, queue_context );
  } else {
    _FMLPL_Release( fmlpl, queue_context );
    status = STATUS_UNAVAILABLE;
  }

  return status;
}



RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Surrender(
  FMLPL_Control        *fmlpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Thread_queue_Heads *heads;
  ISR_lock_Context    lock_context;
  Priority_Node      *priority_node;
  Per_CPU_Control    *cpu_self;
  uint64_t            start, end;

  start = _FMLPL_Get_Nanoseconds();
  priority_node = &(fmlpl->root_node);
  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( executing, &lock_context );
  _Thread_Priority_remove( executing, priority_node, queue_context );
  _Thread_Wait_release_default_critical( executing, &lock_context );

  if ( _FMLPL_Get_owner( fmlpl ) != executing ) {
    _ISR_lock_ISR_enable( &queue_context->Lock_context.Lock_context );
    return STATUS_NOT_OWNER;
  }

  _FMLPL_Acquire_critical( fmlpl, queue_context );
  _FMLPL_Set_owner( fmlpl, NULL );
  heads = fmlpl->Wait_queue.Queue.heads;

  if ( heads == NULL ) {
    cpu_self = _Thread_Dispatch_disable_critical(
      &queue_context->Lock_context.Lock_context
    );
    _FMLPL_Release( fmlpl, queue_context );
    _Thread_queue_Context_clear_priority_updates(queue_context);
    _Thread_Priority_update(queue_context);
    end = _FMLPL_Get_Nanoseconds();
    _Thread_Dispatch_enable( cpu_self );
    return end-start;
  }

  end = _Thread_queue_Surrender(
    &fmlpl->Wait_queue.Queue,
    heads,
    executing,
    queue_context,
    FMLPL_TQ_OPERATIONS
  );
  //end = _FMLPL_Get_Nanoseconds();
  return end-start;
}

RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Can_destroy(
  FMLPL_Control *fmlpl
)
{
  if ( _FMLPL_Get_owner( fmlpl ) != NULL ) {
    return STATUS_RESOURCE_IN_USE;
  }

  return STATUS_SUCCESSFUL;
}

RTEMS_INLINE_ROUTINE void _FMLPL_Destroy(
  FMLPL_Control        *fmlpl,
  Thread_queue_Context *queue_context
)
{
  _FMLPL_Release( fmlpl, queue_context );
  _Thread_queue_Destroy( &fmlpl->Wait_queue );
}

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_FMLPLIMPL_H */
