#ifndef _RTEMS_SCORE_HDGAIMPL_H
#define _RTEMS_SCORE_HDGAIMPL_H

#include <rtems/score/hdga.h>

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
 * @addtogroup ScoreHDGA
 *
 * @{
 */


#define HDGA_TQ_OPERATIONS &_Thread_queue_Operations_TICKET

/**
 * @brief Locks the queue of the sempahore control block
 *
 * @param hdga the sempahore control block
 * @param queue_context queue for locking
 *
 */
RTEMS_INLINE_ROUTINE void _HDGA_Acquire_critical(
  HDGA_Control         *hdga,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Acquire_critical( &hdga->Wait_queue, queue_context );
}

/**
 * @brief Gets the ticket number of the currently executing task
 *
 * @param executing the currently executing task
 * @return The ticket numbe rof the task
 *
 */
RTEMS_INLINE_ROUTINE Ticket_Control _HDGA_Get_Ticket_number(
  Thread_Control *executing
)
{
  return executing->ticket.ticket;
}

/**
 * @brief Incements the current position in the queue by incrementing the
 * 	pointer current_position
 *
 * @param hdga the sempahore control block
 *
 */
RTEMS_INLINE_ROUTINE void _HDGA_Increment_Current_Position(
  HDGA_Control *hdga
)
{
  hdga->current_position = ++hdga->current_position;
  int current_pos = hdga->current_position;

  if ( current_pos == hdga->order_size ) {
	  hdga->current_position = 0;
  }
}

/**
 * @brief Checks if the ticket number of a task is valid by comparing the current ticket
 * 	number in the queue and the ticket number of the task
 *
 * @param hdga the semaphore control block
 * @param executing The executing task
 */
RTEMS_INLINE_ROUTINE bool _HDGA_Has_Valid_ticket(
    HDGA_Control *hdga,
    Thread_Control *executing
)
{
  return executing->ticket.ticket == hdga->ticket_order[hdga->current_position];
}

/**
 * @brief Get the timestamps for the overhead measurements.
 *
 * @return The timestamp.
 *
 */
RTEMS_INLINE_ROUTINE uint64_t _HDGA_Get_Nanoseconds( void )
{
  Timestamp_Control  snapshot_as_timestamp;
  _TOD_Get_zero_based_uptime(&snapshot_as_timestamp);
  return _Timestamp_Get_as_nanoseconds(&snapshot_as_timestamp);
}


/**
 * @brief Releases the queue of the sempahore control block
 *
 * @param hdga the sempahore control block
 * @param queue_context queue for locking
 *
 */
RTEMS_INLINE_ROUTINE void _HDGA_Release(
  HDGA_Control         *hdga,
  Thread_queue_Context *queue_context
)
{
  _Thread_queue_Release( &hdga->Wait_queue, queue_context );
}

/**
 * @brief Gets the owner of the semaphore control block
 *
 * @param hdga the semaphore control block
 * @return The owner of the sempahore control block
 *
 */
RTEMS_INLINE_ROUTINE Thread_Control *_HDGA_Get_owner(
  const HDGA_Control *hdga
)
{
  return hdga->Wait_queue.Queue.owner;
}

/**
 * @brief Sets the owner of the semaphore control block
 *
 * @param hdga the semaphore control block
 * @param executing The owner of the sempahore control block
 *
 */
RTEMS_INLINE_ROUTINE void _HDGA_Set_owner(
  HDGA_Control   *hdga,
  Thread_Control *owner
)
{
  hdga->Wait_queue.Queue.owner = owner;
}

/**
 * @brief Claims ownership of the HDGA sempahore
 *
 * @param hdga the semaphore control block
 * @param executing The owner of the sempahore control block
 * @param queue_context queue for locking
 */
RTEMS_INLINE_ROUTINE Status_Control _HDGA_Claim_ownership(
  HDGA_Control         *hdga,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  uint64_t start, end;
  start = _HDGA_Get_Nanoseconds();
  _HDGA_Set_owner( hdga, executing );
  _HDGA_Release( hdga, queue_context );
  end = _HDGA_Get_Nanoseconds();
  //return end-start;
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Initializes a HDGA control.
 *
 * @param[out] hdga The HDGA control that is initialized.
 * @param scheduler The scheduler for the operation.
 * @param queue_size the size of our ticket array
 * @param executing The currently executing thread.  Ignored in this method.
 * @param initially_locked Indicates whether the HDGA control shall be initally
 *      locked. If it is initially locked, this method returns STATUS_INVALID_NUMBER.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_INVALID_NUMBER The HDGA control is initially locked.
 */
RTEMS_INLINE_ROUTINE Status_Control _HDGA_Initialize(
  HDGA_Control            *hdga,
  const Scheduler_Control *scheduler,
  Priority_Control         queue_size,
  Thread_Control          *executing,
  bool                     initially_locked
)
{

  hdga->order_size = queue_size;
  hdga->current_position = 0;

  if ( initially_locked ) {
    return STATUS_INVALID_NUMBER;
  }

  hdga->ticket_order = _Workspace_Allocate(
    sizeof( *hdga->ticket_order ) * hdga->order_size
  );

  if ( hdga->ticket_order == NULL ) {
    return STATUS_NO_MEMORY;
  }

  _Thread_queue_Object_initialize( &hdga->Wait_queue );
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Waits for the ownership of the HDGA control.
 *
 *
 * @param hdga The HDGA control to get the ownership of.
 * @param executing The currently executing thread.
 * @param queue_context the thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_DEADLOCK A deadlock occured.
 * @retval STATUS_TIMEOUT A timeout occured.
 */

RTEMS_INLINE_ROUTINE Status_Control _HDGA_Wait_for_ownership(
  HDGA_Control         *hdga,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  uint64_t start, end, preq, postq;
  start = _HDGA_Get_Nanoseconds();
 _Thread_queue_Context_set_thread_state(
   queue_context,
   STATES_WAITING_FOR_MUTEX
  );
 _Thread_queue_Context_set_deadlock_callout(
   queue_context,
   _Thread_queue_Deadlock_status
 	 );
 preq = _HDGA_Get_Nanoseconds();
 _Thread_queue_Enqueue(
   &hdga->Wait_queue.Queue,
   HDGA_TQ_OPERATIONS,
   executing,
   queue_context
 );
 postq = _HDGA_Get_Nanoseconds();
 end = _HDGA_Get_Nanoseconds();
 //return (end-start-(postq-preq));
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
RTEMS_INLINE_ROUTINE Status_Control _HDGA_Seize(
  HDGA_Control         *hdga,
  Thread_Control       *executing,
  bool                  wait,
  Thread_queue_Context *queue_context
)
{
  Status_Control  status;
  Thread_Control *owner;

  _HDGA_Acquire_critical( hdga, queue_context );

  owner = _HDGA_Get_owner( hdga );

  if ( owner == NULL && _HDGA_Has_Valid_ticket(hdga, executing)) {
    status = _HDGA_Claim_ownership( hdga, executing, queue_context );
  } else if ( owner == executing ) {
    _HDGA_Release( hdga, queue_context );
    status = STATUS_UNAVAILABLE;
  } else if ( wait ) {
    status = _HDGA_Wait_for_ownership( hdga, executing, queue_context );
  } else {
    _HDGA_Release( hdga, queue_context );
    status = STATUS_UNAVAILABLE;
  }

  return status;
}

/**
 * @brief Surrenders the HDGA control.
 *
 * @param[in, out] hdga The HDGA control to surrender the control of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 *
 * @retval STATUS_SUCCESSFUL The operation succeeded.
 * @retval STATUS_NOT_OWNER The executing thread does not own the HDGA control.
 */
RTEMS_INLINE_ROUTINE Status_Control _HDGA_Surrender(
  HDGA_Control         *hdga,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context
)
{
  Thread_Control   *new_owner;
  uint64_t start, end, preq, postq;
  preq = 0;
  postq = 0;

  _HDGA_Acquire_critical( hdga, queue_context );
  //cpu_self = _Thread_Dispatch_disable_critical(&queue_context->Lock_context.Lock_context );
  start = _HDGA_Get_Nanoseconds();
  if (_HDGA_Get_owner( hdga ) != executing) {
   _HDGA_Release( hdga , queue_context );
  //return 1337;
    return STATUS_NOT_OWNER;
  }
  _Thread_queue_Context_clear_priority_updates( queue_context );
  _HDGA_Increment_Current_Position(hdga);

  new_owner = _Thread_queue_First_locked(
                &hdga->Wait_queue,
		HDGA_TQ_OPERATIONS
	      );
  _HDGA_Set_owner(hdga, new_owner);

  if ( new_owner != NULL ) {
  #if defined(RTEMS_MULTIPROCESSING)
  	if ( _Objects_Is_local_id( new_owner->Object.id ) )
  #endif
  	{
  	}
	if ( !_HDGA_Has_Valid_ticket( hdga, new_owner ) ) {
	  end = _HDGA_Get_Nanoseconds();
	  _HDGA_Set_owner( hdga, NULL );
	  _HDGA_Release( hdga, queue_context );
	  //return (end-start);
	  return STATUS_SUCCESSFUL;
	}

  	end = _Thread_queue_Extract_critical(
  	  &hdga->Wait_queue.Queue,
	  HDGA_TQ_OPERATIONS,
  	  new_owner,
  	  queue_context
  	);

    } else {
        _HDGA_Release( hdga, queue_context );
    }

    end = _HDGA_Get_Nanoseconds();
    return STATUS_SUCCESSFUL;
}

/**
 * @brief Checks if the HDGA control can be destroyed.
 *
 * @param dpcp The HDGA control for the operation.
 *
 * @retval STATUS_SUCCESSFUL The HDGA is currently not used
 *      and can be destroyed.
 * @retval STATUS_RESOURCE_IN_USE The HDGA control is in use,
 *      it cannot be destroyed.
 */
RTEMS_INLINE_ROUTINE Status_Control _HDGA_Can_destroy( HDGA_Control *hdga )
{
  if ( _HDGA_Get_owner( hdga ) != NULL ||
      _Thread_queue_First_locked(
	&hdga->Wait_queue,
	HDGA_TQ_OPERATIONS
  ) != NULL)  {
    return STATUS_RESOURCE_IN_USE;
  }

  return STATUS_SUCCESSFUL;
}

/**
 * @brief Gives the task a ticket number and saves it in our ticket array
 * in the hdga control block. Originally it was possible to have the same task more than
 * one time in the queue, hence the weird if else conidtion.
 *
 * @param[in, out] hdga The HDGA control to surrender the control of.
 * @param[in, out] executing The currently executing thread.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE Status_Control _HDGA_Set_thread(
  HDGA_Control         *hdga,
  Thread_Control       *executing,
  Thread_queue_Context *queue_context,
  int   	        position
)
{
  _HDGA_Acquire_critical( hdga, queue_context );
  if (_HDGA_Get_Ticket_number( executing ) == 0) {
    int posT = position + 1;
    executing->ticket.ticket = posT;
    hdga->ticket_order[position] = executing->ticket.ticket;
  } else {
    hdga->ticket_order[position] = executing->ticket.ticket;
  }
  _HDGA_Release( hdga, queue_context );
  return STATUS_SUCCESSFUL;
}

/**
 * @brief Deletes the HDGA control.
 *
 * @param[in, out] hdga The HDGA control to surrender the control of.
 * @param queue_context The thread queue context.
 */
RTEMS_INLINE_ROUTINE void _HDGA_Destroy(
  HDGA_Control         *hdga,
  Thread_queue_Context *queue_context
)
{
  _HDGA_Release( hdga, queue_context );
  _Thread_queue_Destroy( &hdga->Wait_queue );
  _Workspace_Free( hdga->ticket_order);
}

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* RTEMS_SMP */

#endif /* _RTEMS_SCORE_HDGAIMPL_H */
