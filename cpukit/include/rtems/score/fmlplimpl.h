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

/**
 * @brief Acquires critical according to FMLP-L.
 *
 * @param fmlpl The FMLP-L control for the operation.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _FMLPL_Acquire_critical(
  FMLPL_Control        *fmlpl,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Acquire_critical( &fmlpl->Wait_queue, queue_context );
}

/**
 * @brief Releases according to FMLP-L.
 *
 * @param fmlpl The FMLP-L control for the operation.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _FMLPL_Release(
  FMLPL_Control        *fmlpl,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Release( &fmlpl->Wait_queue, queue_context );
}

/**
 * @brief Gets owner of the FMLP-L control.
 *
 * @param fmlpl The FMLP-L control to get the owner from.
 *
 * @return The owner of the FMLP-L control.
 */
RTEMS_INLINE_ROUTINE Thread_Control *_FMLPL_Get_owner(
  const FMLPL_Control *fmlpl
)
{
  return fmlpl->Wait_queue.Queue.owner;
}

/**
 * @brief Sets owner of the FMLP-L control.
 *
 * @param[out] fmlpl The FMLP-L control to set the owner of.
 * @param owner The desired new owner for @a fmlpl.
 */
RTEMS_INLINE_ROUTINE void _FMLPL_Set_owner(
  FMLPL_Control  *fmlpl,
  Thread_Control *owner
)
{
  fmlpl->Wait_queue.Queue.owner = owner;
}

/**
 * @brief Gets priority of the FMLP-L control.
 *
 * @param fmlpl The fmlpl control to get the priority from.
 * @param scheduler The corresponding scheduler.
 *
 * @return The priority of the FMLP-L control.
 */
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

/**
 * @brief Changes the priority of the owner.
 *
 * @param new_prio THe new priority of the owner.
 * @param fmlpl The FMLP-L control for the operation.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Change_Owner_Priority(
  Priority_Control      new_prio,
  FMLPL_Control        *fmlpl,
  Thread_queue_Context *queue_context
)
{
  Thread_Control  *owner;
  ISR_lock_Context lock_context;
  Priority_Node   *priority_node;

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

/**
 * @brief Gets the minimum priority from priority array of the FMLP-L control block.
 *
 * @param fmlpl The FMLP-L control to get the array.
 *
 * @retval Minimum priority from the array
 */
RTEMS_INLINE_ROUTINE Priority_Control _FMLPL_Get_Min_Priority(
  FMLPL_Control *fmlpl
)
{
  int		   i;
  int 		   fs;
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

/**
 * @brief Inserts priority into the priority array of the FMLP-L control.
 *
 * @param fmlpl The FMLP-L control, to get the priority array.
 * @param executing The currently executing thread.
 *
 * @retval 0 The operation succeeded.
 * @retval 1 No free spot available.
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

/**
 * @brief Removes priority from the priority array in the FMLP-L control.
 *
 * @param fmlpl The FMLP-L control to get the array.
 *
 * @retval 0 The operation succeeded.
 */
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

/**
 * @brief Adds thread to priority array, triggers priority changes to owner thread.
 *
 * @param fmlpl FMLP-L to get the array form.
 * @param executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 */
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

/**
 * @brief Claims ownership of the FMLP-L control.
 *
 * @param fmlpl The FMLP-L control to claim the ownership of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the executing
 *      thread exceeds the ceiling priority.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Claim_ownership(
  FMLPL_Control        *fmlpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{

  ISR_lock_Context  lock_context;
  Priority_Node    *priority_node;

  _FMLPL_Set_owner( fmlpl, executing );
  priority_node = &( fmlpl->root_node );
  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( executing, &lock_context );
  _Priority_Node_initialize(
    priority_node,
    SCHEDULER_PRIORITY_MAP( _FMLPL_Get_Min_Priority( fmlpl ))
  );
  _Thread_Wait_release_default_critical( executing, &lock_context );

  _FMLPL_Release( fmlpl, queue_context );

  return RTEMS_SUCCESSFUL;
}

/**
 * @brief Initializes a FMLP-L control.
 *
 * @param[out] fmlpl The FMLP-L control that is initialized.
 * @param scheduler The scheduler for the operation.
 * @param ceiling_priority
 * @param executing The currently executing thread.  Ignored in this method.
 * @param initially_locked Indicates whether the FMLP-L control shall be initially
 *      locked. If it is initially locked, this method returns STATUS_INVALID_NUMBER.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_INVALID_NUMBER The FMLP-L control is initially locked.
 * @retval STATUS_NO_MEMORY There is not enough memory to allocate.
 */
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

/**
 * @brief Waits for the ownership of the FMLP-L control.
 *
 * @param[in, out] fmlpl The FMLP-L control to get the ownership of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context the thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the
 *      currently executing thread exceeds the ceiling priority.
 * @retval STATUS_DEADLOCK A deadlock occured.
 * @retval STATUS_TIMEOUT A timeout occured.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Wait_for_ownership(
  FMLPL_Control        *fmlpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
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
  _Thread_queue_Enqueue(
    &fmlpl->Wait_queue.Queue,
    FMLPL_TQ_OPERATIONS,
    executing,
    queue_context
  );
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

  return RTEMS_SUCCESSFUL;
}

/**
 * @brief Seizes the FMLP-L control.
 *
 * @param[in, out] fmlpl The FMLP-L control to seize the control of.
 * @param[in, out] executing The currently executing thread.
 * @param wait Indicates whether the calling thread is willing to wait.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the executing
 *      thread exceeds the ceiling priority.
 * @retval STATUS_UNAVAILABLE The executing thread is already the owner of
 *      the FMLP-L control.  Seizing it is not possible.
 */
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

/**
 * @brief Surrenders the FMLP-L control.
 *
 * @param[in, out] fmlpl The FMLP-L control to surrender the control of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_NOT_OWNER The executing thread does not own the FMLP-L control.
 */
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
    _Thread_Dispatch_enable( cpu_self );
    return RTEMS_SUCCESSFUL;
  }

  _Thread_queue_Surrender(
    &fmlpl->Wait_queue.Queue,
    heads,
    executing,
    queue_context,
    FMLPL_TQ_OPERATIONS
  );

  return RTEMS_SUCCESSFUL;
}

/**
 * @brief Checks if the FMLP-L control can be destroyed.
 *
 * @param fmlpl The FMLP-L control for the operation.
 *
 * @retval STATUS_SUCCESSFUL The FMLP-L is currently not used
 *      and can be destroyed.
 * @retval STATUS_RESOURCE_IN_USE The FMLP-L control is in use,
 *      it cannot be destroyed.
 */
RTEMS_INLINE_ROUTINE Status_Control _FMLPL_Can_destroy(
  FMLPL_Control *fmlpl
)
{
  if ( _FMLPL_Get_owner( fmlpl ) != NULL ) {
    return STATUS_RESOURCE_IN_USE;
  }

  return STATUS_SUCCESSFUL;
}

/**
 * @brief Destroys the FMLP-L control
 *
 * @param[in, out] The fmlpl that is about to be destroyed.
 * @param queue_context The thread queue context.
 */
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
