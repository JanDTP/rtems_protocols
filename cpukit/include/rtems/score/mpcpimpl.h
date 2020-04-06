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

#ifndef _RTEMS_SCORE_MPCPIMPL_H
#define _RTEMS_SCORE_MPCPIMPL_H


#include <rtems/score/mpcp.h>

#if defined(RTEMS_SMP)

#include <rtems/score/assert.h>
#include <rtems/score/status.h>
#include <rtems/score/threadqimpl.h>
#include <rtems/score/watchdogimpl.h>
#include <rtems/score/wkspace.h>
#include <rtems/score/schedulerimpl.h>
#include <rtems/score/threadimpl.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @addtogroup ScoreMPCP
 *
 * @{
 */

#define MPCP_TQ_OPERATIONS &_Thread_queue_Operations_priority

RTEMS_INLINE_ROUTINE void _MPCP_Acquire_critical(
        MPCP_Control         *mpcp,
        Thread_queue_Context *queue_context
)
{
    _Thread_queue_Acquire_critical( &mpcp->Wait_queue, queue_context );
}

RTEMS_INLINE_ROUTINE void _MPCP_Release(
  MPCP_Control         *mpcp,
  Thread_queue_Context *queue_context
)
{
    _Thread_queue_Release( &mpcp->Wait_queue, queue_context );
}

RTEMS_INLINE_ROUTINE Thread_Control *_MPCP_Get_owner(
        const MPCP_Control *mpcp
)
{
    return mpcp->Wait_queue.Queue.owner;
}

RTEMS_INLINE_ROUTINE void _MPCP_Set_owner(
        MPCP_Control   *mpcp,
        Thread_Control *owner
)
{
    mpcp->Wait_queue.Queue.owner = owner;
}

RTEMS_INLINE_ROUTINE Priority_Control _MPCP_Get_priority(
        const MPCP_Control      *mpcp,
        const Scheduler_Control *scheduler
)
{
    uint32_t scheduler_index;

    scheduler_index = _Scheduler_Get_index( scheduler );
    return mpcp ->ceiling_priorities[scheduler_index];
}

RTEMS_INLINE_ROUTINE void _MPCP_Set_priority(
        MPCP_Control            *mpcp,
        const Scheduler_Control *scheduler,
        Priority_Control         new_priority
)
{
    uint32_t scheduler_index;

    scheduler_index = _Scheduler_Get_index( scheduler );
    mpcp->ceiling_priorities[ scheduler_index ] = new_priority;
}

RTEMS_INLINE_ROUTINE Status_Control _MPCP_Raise_priority(
        MPCP_Control         *mpcp,
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
    ceiling_priority = _MPCP_Get_priority( mpcp, scheduler );

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

RTEMS_INLINE_ROUTINE void _MPCP_Remove_priority(
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

RTEMS_INLINE_ROUTINE void _MPCP_Replace_priority(
        MPCP_Control   *mpcp,
        Thread_Control *thread,
        Priority_Node  *ceiling_priority
)
{
    ISR_lock_Context lock_context;

    _Thread_Wait_acquire_default( thread, &lock_context );
    _Thread_Priority_replace(
            thread,
            ceiling_priority,
            &mpcp->Ceiling_priority
    );
    _Thread_Wait_release_default( thread, &lock_context );
}

RTEMS_INLINE_ROUTINE Status_Control _MPCP_Claim_ownership(
        MPCP_Control         *mpcp,
        Thread_Control       *executing,
        Thread_queue_Context *queue_context
)
{
    Status_Control   status;
    Per_CPU_Control *cpu_self;

    status = _MPCP_Raise_priority(
            mpcp,
            executing,
            &mpcp->Ceiling_priority,
            queue_context
    );

    if ( status != STATUS_SUCCESSFUL ) {
        _MPCP_Release( mpcp, queue_context );
        return status;
    }

    _MPCP_Set_owner( mpcp, executing );
    cpu_self = _Thread_queue_Dispatch_disable( queue_context );
    _MPCP_Release( mpcp, queue_context );
    _Thread_Priority_update( queue_context );
    _Thread_Dispatch_enable( cpu_self );

    return RTEMS_SUCCESSFUL;
}

RTEMS_INLINE_ROUTINE Status_Control _MPCP_Initialize(
        MPCP_Control            *mpcp,
        const Scheduler_Control *scheduler,
        Priority_Control         ceiling_priority,
        Thread_Control          *executing,
        bool                     initially_locked
)
{
  (void) executing;
    uint64_t scheduler_count = _Scheduler_Count;
    uint32_t i;

    if ( initially_locked ) {
        return STATUS_INVALID_NUMBER;
    }

    mpcp->ceiling_priorities = (Priority_Control *)_Workspace_Allocate(
            sizeof( *mpcp->ceiling_priorities ) * scheduler_count
    );
    if ( mpcp->ceiling_priorities == NULL ) {
        return STATUS_NO_MEMORY;
    }

    for ( i = 0 ; i < scheduler_count ; ++i ) {
        const Scheduler_Control *scheduler_of_index;

        scheduler_of_index = &_Scheduler_Table[ i ];

        if ( scheduler != scheduler_of_index ) {
            mpcp->ceiling_priorities[ i ] =
                   _Scheduler_Map_priority( scheduler_of_index, 1);
        } else {
            mpcp->ceiling_priorities[ i ] = ceiling_priority;
        }
    }

    _Thread_queue_Object_initialize( &mpcp->Wait_queue );
    return STATUS_SUCCESSFUL;
}

RTEMS_INLINE_ROUTINE Status_Control _MPCP_Wait_for_ownership(
  MPCP_Control         *mpcp,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{

  Status_Control status;
  Priority_Node  ceiling_priority;

  _Thread_queue_Context_set_thread_state(
    queue_context,
    STATES_WAITING_FOR_SEMAPHORE
  );

  _Thread_queue_Context_set_deadlock_callout(
    queue_context,
    _Thread_queue_Deadlock_status
  );

  _Thread_queue_Enqueue(
    &mpcp->Wait_queue.Queue,
    MPCP_TQ_OPERATIONS,
    executing,
    queue_context
  );

  status = _MPCP_Raise_priority(
    mpcp,
    executing,
    &ceiling_priority,
    queue_context
  );

  if ( status != STATUS_SUCCESSFUL ) {
    _MPCP_Release( mpcp, queue_context );
    return status;
  }
  _MPCP_Replace_priority( mpcp, executing, &ceiling_priority );

  return status;
}

RTEMS_INLINE_ROUTINE Status_Control _MPCP_Seize(
        MPCP_Control         *mpcp,
        Thread_Control       *executing,
        bool                  wait,
        Thread_queue_Context *queue_context
)
{
    Status_Control  status;
    Thread_Control *owner;

    _MPCP_Acquire_critical( mpcp, queue_context );

    owner = _MPCP_Get_owner( mpcp );

    if ( owner == NULL ) {
        status = _MPCP_Claim_ownership( mpcp, executing, queue_context );
    } else if ( owner == executing ) {
        _MPCP_Release( mpcp, queue_context );
        status = STATUS_UNAVAILABLE;
    } else if ( wait ) {
        status = _MPCP_Wait_for_ownership( mpcp, executing, queue_context );
    } else {
        _MPCP_Release( mpcp, queue_context );
        status = STATUS_UNAVAILABLE;
    }

    return status;
}

RTEMS_INLINE_ROUTINE Status_Control _MPCP_Surrender(
        MPCP_Control         *mpcp,
        Thread_Control       *executing,
        Thread_queue_Context *queue_context
)
{
    Thread_queue_Heads *heads;

    if ( _MPCP_Get_owner( mpcp ) != executing ) {
        _ISR_lock_ISR_enable( &queue_context->Lock_context.Lock_context );
        return STATUS_NOT_OWNER;
    }

    _MPCP_Acquire_critical( mpcp, queue_context );

    _MPCP_Set_owner( mpcp, NULL );
    _MPCP_Remove_priority( executing, &mpcp->Ceiling_priority, queue_context );

    heads = mpcp->Wait_queue.Queue.heads;

    if ( heads == NULL ) {
        Per_CPU_Control *cpu_self;

        cpu_self = _Thread_Dispatch_disable_critical(
                     &queue_context->Lock_context.Lock_context
        );
        _MPCP_Release( mpcp, queue_context );
        _Thread_Priority_update( queue_context );
        _Thread_Dispatch_enable( cpu_self );
        return STATUS_SUCCESSFUL;
    }

    _Thread_queue_Surrender(
            &mpcp->Wait_queue.Queue,
            heads,
            executing,
            queue_context,
            MPCP_TQ_OPERATIONS
    );
    return STATUS_SUCCESSFUL;
}

RTEMS_INLINE_ROUTINE Status_Control _MPCP_Can_destroy( MPCP_Control *mpcp )
{
    if ( _MPCP_Get_owner( mpcp ) != NULL ) {
        return STATUS_RESOURCE_IN_USE;
    }

    return STATUS_SUCCESSFUL;
}

RTEMS_INLINE_ROUTINE void _MPCP_Destroy(
        MPCP_Control         *mpcp,
        Thread_queue_Context *queue_context
)
{
    _MPCP_Release( mpcp, queue_context );
    _Thread_queue_Destroy( &mpcp->Wait_queue );
    _Workspace_Free( mpcp->ceiling_priorities );
}

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_MPCPIMPL_H */
