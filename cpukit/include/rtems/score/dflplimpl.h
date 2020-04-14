


#ifndef _RTEMS_SCORE_DFLPLIMPL_H
#define _RTEMS_SCORE_DFLPLIMPL_H

#include "dflpl.h"

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
 * @addtogroup ScoreDFLPL
 *
 * @{
 */

#define DFLPL_TQ_OPERATIONS &_Thread_queue_Operations_FIFO

/**
 * @brief Migrates Thread to an synchronization processor.
 *
 * @param executing The executing Thread
 * @param dflpl the semaphore control block
 * @param migration_priority the priority of the task in the foreign scheduler instance
 */
RTEMS_INLINE_ROUTINE void _DFLPL_Migrate(
    Thread_Control *executing,
    DFLPL_Control  *dflpl,
    Priority_Node  *migration_priority
)
{

  _Scheduler_Migrate_To(executing, dflpl->pu, migration_priority);
}

/**
 * @brief Migrates Thread back to the application processor.
 *
 * @param executing The executing Thread
 * @param dflpl the semaphore control block
 */
RTEMS_INLINE_ROUTINE void _DFLPL_Migrate_Back(
  Thread_Control *executing,
  DFLPL_Control  *dflpl
)
{
  _Scheduler_Migrate_Back( executing, dflpl->pu );

}

/**
 * @brief Acquires the lock for the queue_context
 *
 * @param dflpl The DFLPL control for the operation.
 * @param queue_contest the queue_context of the semphore
 */
RTEMS_INLINE_ROUTINE void _DFLPL_Acquire_critical(
  DFLPL_Control        *dflpl,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Acquire_critical( &dflpl->Wait_queue, queue_context );
}


/**
 * @brief Releases the lock for the queue_context
 *
 * @param dflpl The DFLPL control for the operation.
 * @param queue_contest the queue_context of the semphore
 */
RTEMS_INLINE_ROUTINE void _DFLPL_Release(
  DFLPL_Control        *dflpl,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Release( &dflpl->Wait_queue, queue_context );
}

/**
 * @brief Sets the synchronization CPU of the DFLPL Control
 *
 * @param dpcp The semaphore control block-
 * @param cpu The synchronization processor it changes to.
 * @param queue_context struct to secure sempahore access
 */

RTEMS_INLINE_ROUTINE void _DFLPL_Set_CPU(
  DFLPL_Control        *dflpl,
  Per_CPU_Control      *cpu,
  Thread_queue_Context *queue_context
)
{
  _DFLPL_Acquire_critical(dflpl, queue_context);
  dflpl->pu = cpu;
  _DFLPL_Release(dflpl, queue_context);

}
/**
 * @brief Gets the current owner of the semaphore
 * @param dflpl The DFLPL control for the operation.
 *
 * @return The current owner otherwise NULL
 */
RTEMS_INLINE_ROUTINE Thread_Control *_DFLPL_Get_owner(
  const DFLPL_Control *dflpl
)
{
  return dflpl->Wait_queue.Queue.owner;
}

/**
 * @brief Sets the current owner of the semaphore in the semaphore data structure
 *
 * @param dflpl The DFLPL control for the operation.
 * @param executing The new owner
 */
RTEMS_INLINE_ROUTINE void _DFLPL_Set_owner(
  DFLPL_Control  *dflpl,
  Thread_Control *owner
)
{
  dflpl->Wait_queue.Queue.owner = owner;
}

/**
 * @brief Gets the priority of the executing task in its home scheduler instance
 *
 * @param executing The new owner
 */
RTEMS_INLINE_ROUTINE Priority_Control _DFLPL_Get_priority(
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
 * @brief Changes the priority of the owner of the semaphore in a different scheduler instance
 *
 * @param executing The owner
 * @param new_prio the new priority of the owning task
 * @param dflpl the smepahore control structure
 * @param queue_context the queue_context of the semaphore
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_Change_Owner_Priority(
  Thread_Control       *executing,
  Priority_Control      new_prio,
  DFLPL_Control        *dflpl,
  Thread_queue_Context *queue_context
)
{
  Thread_Control  *owner;
  ISR_lock_Context lock_context;
  Priority_Node   *priority_node;

  priority_node = &(dflpl->root_node);
  owner = _DFLPL_Get_owner(dflpl);
  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( owner, &lock_context );
  _Priority_Node_initialize(priority_node, SCHEDULER_PRIORITY_MAP(new_prio));
  _Scheduler_Change_migration_priority(owner, dflpl->pu, priority_node);
  _Thread_Wait_release_default_critical( owner, &lock_context );
  return RTEMS_SUCCESSFUL;
}


/**
 * @brief Gets the minimum priority of the priority array of the dflpl semaphore
 *
 * @param dflpl The DFLPL control for the operation.
 *
 */
RTEMS_INLINE_ROUTINE Priority_Control _DFLPL_Get_Min_Priority(
  DFLPL_Control *dflpl
)
{
  int		   i;
  int 		   fs;
  Priority_Control min_prio;

  min_prio = PRIORITY_DEFAULT_MAXIMUM;
  fs = dflpl->first_free_slot;

  for ( i = 0; i < fs; i++ ) {
    if ( dflpl->priority_array[i] < min_prio ) {
      min_prio = dflpl->priority_array[i];
     }
  }

  return min_prio;
}

/**
 * @brief Inserts priority into the priority array of the DFLP-L control.
 *
 * @param dflp The DFLP-L control, to get the priority array.
 * @param executing The currently executing thread.
 *
 * @retval 0 The operation succeeded.
 * @retval 1 No free spot available.
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_Insert(
  DFLPL_Control  *dflpl,
  Thread_Control *executing
)
{
  //negative slot number
  if ( dflpl->first_free_slot < 0 ) {
    return 1;
  }

   //slot number over maximum
  if ( dflpl->first_free_slot > 15 ) {
    return 1;
  }

  //slot already in use
  if ( dflpl->priority_array[dflpl->first_free_slot] != 0 ) {
    return 1;
  }

  //adding the priority of the waiting thread
  dflpl->priority_array[dflpl->first_free_slot] = _DFLPL_Get_priority( executing );
  if ( dflpl->first_free_slot==15 ) {
  }
  dflpl->first_free_slot++;
  return 0;
}

/**
 * @brief Removes priority from the priority array
 *
 * @param dflpl The DFLP-L control for the operation.
 *
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_remove(
  DFLPL_Control *dflpl
)
{
  if ( dflpl->first_free_slot < 0 ) {
    return 1;
  }

  if ( dflpl->first_free_slot > 15 ) {
    return 1;
  }

  if ( dflpl->priority_array[dflpl->first_free_slot] != 0 ) {
    return 1;
  }
  int i, fs;
  Priority_Control head; //not in use
  head = dflpl->priority_array[0]; //not in use
  fs = dflpl->first_free_slot;
  for ( i = 0; i < fs; i++ ) {
      dflpl->priority_array[i] = dflpl->priority_array[i+1];
  }
  dflpl->first_free_slot--;
  fs = dflpl->first_free_slot;
  dflpl->priority_array[fs] = 0;
  return 0;
}

/**
 * @brief Adds thread to priority array, triggers priority changes to owner thread.
 *
 * @param dflpl DFLP-L to get the array form.
 * @param executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_add(
  DFLPL_Control        *dflpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Priority_Control min, newmin;
  min = _DFLPL_Get_Min_Priority( dflpl );
  _DFLPL_Insert( dflpl, executing );

  newmin = _DFLPL_Get_Min_Priority( dflpl );
  if ( newmin < min ) {
    _DFLPL_Change_Owner_Priority( executing, newmin, dflpl, queue_context );
  }
  return RTEMS_SUCCESSFUL;
}

/**
 * @brief Gets the minimum priority of the priority array of the DFLP-L semaphore
 *
 * @param dflpl The DFLP-L control for the operation.
 * @param executing The executing task
 * @param queue_context struct to secure sempahore access
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 *
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_Claim_ownership(
  DFLPL_Control        *dflpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Per_CPU_Control *cpu_self;
  ISR_lock_Context lock_context;
  Priority_Node   *priority_node;

  cpu_self = _Thread_queue_Dispatch_disable( queue_context );
  _DFLPL_Set_owner( dflpl, executing );
  priority_node = &(dflpl->root_node);
  _Thread_queue_Context_clear_priority_updates( queue_context );
  _Thread_Wait_acquire_default_critical( executing, &lock_context );
  _Priority_Node_initialize(priority_node, _DFLPL_Get_priority(executing));
  _DFLPL_Migrate(executing, dflpl, priority_node);

  _Thread_Wait_release_default_critical( executing, &lock_context );
  _DFLPL_Release( dflpl, queue_context );
  _Thread_Dispatch_enable( cpu_self );
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Initializes a DFLPL control. The sychronization processor is set to CPU#1
 *	by default.
 *
 * @param[out] dflpl The DFLPL control that is initialized.
 * @param scheduler The scheduler for the operation.
 * @param ceiling_priority
 * @param executing The currently executing thread.  Ignored in this method.
 * @param initially_locked Indicates whether the DFLPL control shall be initally
 *      locked. If it is initially locked, this method returns STATUS_INVALID_NUMBER.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_INVALID_NUMBER The DFLPL control is initially locked.
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_Initialize(
  DFLPL_Control           *dflpl,
  const Scheduler_Control *scheduler,
  Priority_Control         ceiling_priority,
  Thread_Control          *executing,
  bool                     initially_locked
)
{
  int i;

  if ( initially_locked ) {
      return STATUS_INVALID_NUMBER;
  }

  dflpl->pu = _Per_CPU_Get_by_index(1);
  _Thread_queue_Object_initialize( &dflpl->Wait_queue );

  dflpl->priority_array = _Workspace_Allocate(
      sizeof( *dflpl->priority_array ) * 16 );

  for ( i = 0; i < 16; i++ ) {
    dflpl->priority_array[i] = 0;
  }
  dflpl->first_free_slot = 0;

  Priority_Node *priority_node;
  priority_node = &( dflpl->root_node );

  return STATUS_SUCCESSFUL;
}

/**
 * @brief Waits for the ownership of the DFLP-L control.
 *
 * @param[in, out] dflpl The DFLP-L control to get the ownership of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context the thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_MUTEX_CEILING_VIOLATED The wait priority of the
 *      currently executing thread exceeds the ceiling priority.
 * @retval STATUS_DEADLOCK A deadlock occured.
 * @retval STATUS_TIMEOUT A timeout occured.
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_Wait_for_ownership(
  DFLPL_Control        *dflpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Per_CPU_Control *cpu_self;
  ISR_lock_Context lock_context;
  Priority_Node   *priority_node;
  Priority_Control new_high_prio;

  cpu_self = _Thread_Dispatch_disable_critical( &queue_context->Lock_context.Lock_context );
  _DFLPL_add( dflpl, executing, queue_context );

  _Thread_queue_Context_set_thread_state(
    queue_context,
    STATES_WAITING_FOR_MUTEX
  );
  _Thread_queue_Context_set_enqueue_do_nothing_extra( queue_context );
  _Thread_queue_Context_set_deadlock_callout(
    queue_context,
    _Thread_queue_Deadlock_status
  );
  _Thread_queue_Enqueue2(
    &dflpl->Wait_queue.Queue,
    DFLPL_TQ_OPERATIONS,
    executing,
    queue_context,
    cpu_self
  );

  _DFLPL_Acquire_critical( dflpl, queue_context );

  _DFLPL_remove( dflpl );
  new_high_prio = _DFLPL_Get_Min_Priority( dflpl );

  priority_node = &( dflpl->root_node );
  _DFLPL_Release( dflpl, queue_context );
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Seizes the semaphore. Triggers the subroutines wait for semaphore and claim
 *
 * @param dflpl The DFLPL control for the operation.
 * @param executing The executing task
 * @param queue_context struct to secure sempahore access
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_UNAVAVILABLE Seizing not possible.
 *
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_Seize(
  DFLPL_Control        *dflpl,
  Thread_Control       *executing,
  bool                  wait,
  Thread_queue_Context *queue_context
)
{
  Status_Control  status;
  Thread_Control *owner;

  _DFLPL_Acquire_critical( dflpl, queue_context );

  owner = _DFLPL_Get_owner( dflpl );

  if ( owner == NULL ) {
    status = _DFLPL_Claim_ownership( dflpl, executing, queue_context );
  } else if ( owner == executing ) {
    _DFLPL_Release( dflpl, queue_context );
    status = STATUS_UNAVAILABLE;
  } else if ( wait ) {
    status = _DFLPL_Wait_for_ownership( dflpl, executing, queue_context );
  } else {
    _DFLPL_Release( dflpl, queue_context );
    status = STATUS_UNAVAILABLE;
  }

  return status;
}

/**
 * @brief Surrenders the DFLP-L control.
 *
 * @param[in, out] dflpl The DFLP-L control to surrender the control of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_NOT_OWNER The executing thread does not own the DFLP-L control.
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_Surrender(
  DFLPL_Control        *dflpl,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Thread_queue_Heads *heads;
  ISR_lock_Context    lock_context;
  Priority_Node      *priority_node;
  Per_CPU_Control    *cpu_self;

  cpu_self = _Thread_Dispatch_disable_critical( &queue_context->Lock_context.Lock_context );
  priority_node =    &( dflpl->root_node );
  _Thread_queue_Context_clear_priority_updates( queue_context );

  if ( _DFLPL_Get_owner( dflpl ) != executing ) {
    _ISR_lock_ISR_enable( &queue_context->Lock_context.Lock_context );
    return STATUS_NOT_OWNER;
  }

  _DFLPL_Acquire_critical( dflpl, queue_context );
  _DFLPL_Set_owner( dflpl, NULL );
  heads = dflpl->Wait_queue.Queue.heads;

  if ( heads == NULL ) {
    _DFLPL_Release( dflpl, queue_context );
    _Thread_queue_Context_clear_priority_updates( queue_context );
    _Thread_Wait_acquire_default_critical( executing, &lock_context );
    _DFLPL_Migrate_Back( executing, dflpl );
    _Thread_Wait_release_default_critical( executing, &lock_context );
    _Thread_Dispatch_enable( cpu_self );
    return RTEMS_SUCCESSFUL;
  }

  _Priority_Node_initialize(
    priority_node,
    SCHEDULER_PRIORITY_MAP( _DFLPL_Get_Min_Priority( dflpl ) )
  );

  _Thread_queue_Surrender_and_Migrate(
    &dflpl->Wait_queue.Queue,
    heads,
    executing,
    queue_context,
    DFLPL_TQ_OPERATIONS,
    dflpl->pu,
    priority_node
  );

  _Thread_Wait_acquire_default_critical( executing, &lock_context );
  _DFLPL_Migrate_Back( executing, dflpl );
  _Thread_Wait_release_default_critical( executing, &lock_context );
  _Thread_Dispatch_enable( cpu_self );

  return STATUS_SUCCESSFUL;
}

/**
 * @brief Checks if the DFLP-L control can be destroyed.
 *
 * @param dflpl The DFLP-L control for the operation.
 *
 * @retval STATUS_SUCCESSFUL The DFLP-L is currently not used
 *      and can be destroyed.
 * @retval STATUS_RESOURCE_IN_USE The DFLP-L control is in use,
 *      it cannot be destroyed.
 */
RTEMS_INLINE_ROUTINE Status_Control _DFLPL_Can_destroy(
  DFLPL_Control *dflpl
)
{
  if ( _DFLPL_Get_owner( dflpl ) != NULL ) {
    return STATUS_RESOURCE_IN_USE;
  }
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Destroys the DFLP-L control
 *
 * @param[in, out] The dflpl that is about to be destroyed.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _DFLPL_Destroy(
  DFLPL_Control        *dflpl,
  Thread_queue_Context *queue_context
)
{
  _DFLPL_Release( dflpl, queue_context );
  _Thread_queue_Destroy( &dflpl->Wait_queue );
}

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_DFLPLIMPL_H */
