/**
 * @file
 *
 * @brief rtems_semaphore_create
 * @ingroup ClassicSem Semaphores
 */

/*
 *  COPYRIGHT (c) 1989-2014.
 *  On-Line Applications Research Corporation (OAR).
 *
 *  The license and distribution terms for this file may be
 *  found in the file LICENSE in this distribution or at
 *  http://www.rtems.org/license/LICENSE.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <rtems/rtems/semimpl.h>
#include <rtems/rtems/attrimpl.h>
#include <rtems/rtems/statusimpl.h>
#include <rtems/rtems/support.h>
#include <rtems/rtems/tasksimpl.h>
#include <rtems/score/schedulerimpl.h>
#include <rtems/score/sysstate.h>
#include <rtems/sysinit.h>

#define SEMAPHORE_KIND_MASK ( RTEMS_SEMAPHORE_CLASS | RTEMS_INHERIT_PRIORITY \
  | RTEMS_PRIORITY_CEILING | RTEMS_MULTIPROCESSOR_RESOURCE_SHARING \
  | RTEMS_DISTRIBUTED_PRIORITY_CEILING | RTEMS_FLEXIBLE_MULTIPROCESSOR_LOCKING_SHORT \
  | RTEMS_FLEXIBLE_MULTIPROCESSOR_LOCKING_LONG | RTEMS_HYPERPERIOD_DEPENDENCY_GRAPH_APPROACH \
  | RTEMS_DISTRIBUTED_FLEXIBLE_LOCKING_LONG | RTEMS_MULTIPROCESSOR_PRIORITY_CEILING)

rtems_status_code rtems_semaphore_create(
  rtems_name           name,
  uint32_t             count,
  rtems_attribute      attribute_set,
  rtems_task_priority  priority_ceiling,
  rtems_id            *id
)
{
  Semaphore_Control       *the_semaphore;
  Thread_Control          *executing;
  Status_Control           status;
  rtems_attribute          maybe_global;
  rtems_attribute          mutex_with_protocol;
  Semaphore_Variant        variant;
  const Scheduler_Control *scheduler;
  bool                     valid;
  Priority_Control         priority;

  if ( !rtems_is_name_valid( name ) )
    return RTEMS_INVALID_NAME;

  if ( !id )
    return RTEMS_INVALID_ADDRESS;

#if defined(RTEMS_MULTIPROCESSING)
  if (
    _Attributes_Is_global( attribute_set )
      && !_System_state_Is_multiprocessing
  ) {
    return RTEMS_MP_NOT_CONFIGURED;
  }
#endif

  /* Attribute subset defining a potentially global semaphore variant */
  maybe_global = attribute_set & SEMAPHORE_KIND_MASK;

  /* Attribute subset defining a mutex variant with a locking protocol */
  mutex_with_protocol =
    attribute_set & ( SEMAPHORE_KIND_MASK | RTEMS_GLOBAL | RTEMS_PRIORITY );

  if ( maybe_global == RTEMS_COUNTING_SEMAPHORE ) {
    variant = SEMAPHORE_VARIANT_COUNTING;
  } else if ( count > 1 ) {
    /*
     * The remaining variants are all binary semphores, thus reject an invalid
     * count value.
     */
    return RTEMS_INVALID_NUMBER;
  } else if ( maybe_global == RTEMS_SIMPLE_BINARY_SEMAPHORE ) {
    variant = SEMAPHORE_VARIANT_SIMPLE_BINARY;
  } else if ( maybe_global == RTEMS_BINARY_SEMAPHORE ) {
    variant = SEMAPHORE_VARIANT_MUTEX_NO_PROTOCOL;
  } else if (
    mutex_with_protocol
      == ( RTEMS_BINARY_SEMAPHORE | RTEMS_PRIORITY | RTEMS_INHERIT_PRIORITY )
  ) {
    variant = SEMAPHORE_VARIANT_MUTEX_INHERIT_PRIORITY;
  } else if (
    mutex_with_protocol
      == ( RTEMS_BINARY_SEMAPHORE | RTEMS_PRIORITY | RTEMS_PRIORITY_CEILING )
  ) {
    variant = SEMAPHORE_VARIANT_MUTEX_PRIORITY_CEILING;
  } else if (
    mutex_with_protocol
      == ( RTEMS_BINARY_SEMAPHORE | RTEMS_MULTIPROCESSOR_RESOURCE_SHARING )
  ) {
#if defined(RTEMS_SMP)
    variant = SEMAPHORE_VARIANT_MRSP;
#else
    /*
     * On uni-processor configurations the Multiprocessor Resource Sharing
     * Protocol is equivalent to the Priority Ceiling Protocol.
     */
    variant = SEMAPHORE_VARIANT_MUTEX_PRIORITY_CEILING;
#endif
  } else if (
      mutex_with_protocol
	== ( RTEMS_BINARY_SEMAPHORE | RTEMS_DISTRIBUTED_PRIORITY_CEILING |  \
	    RTEMS_GLOBAL)
    ) {
  #if defined(RTEMS_SMP)
      variant = SEMAPHORE_VARIANT_DPCP;
  #else
      /*
       * Use normal PCP on uni-processor
       */
      variant = SEMAPHORE_VARIANT_MUTEX_PRIORITY_CEILING;
  #endif
    }else if (
      mutex_with_protocol == (RTEMS_BINARY_SEMAPHORE | \
	  RTEMS_FLEXIBLE_MULTIPROCESSOR_LOCKING_SHORT | RTEMS_FIFO | \
          RTEMS_GLOBAL )
    ) {
  #if defined(RTEMS_SMP)
      variant = SEMAPHORE_VARIANT_FMLPS;
  #else
      return RTEMS_MP_NOT_CONFIGURED;
  #endif
    } else if (
	mutex_with_protocol == (RTEMS_BINARY_SEMAPHORE | \
	RTEMS_FLEXIBLE_MULTIPROCESSOR_LOCKING_LONG | RTEMS_FIFO | \
	    RTEMS_GLOBAL )
      ) {
    #if defined(RTEMS_SMP)
	variant = SEMAPHORE_VARIANT_FMLPL;
    #else
	return RTEMS_MP_NOT_CONFIGURED;
    #endif
    } else if (
	mutex_with_protocol
	  == ( RTEMS_BINARY_SEMAPHORE | RTEMS_DISTRIBUTED_FLEXIBLE_LOCKING_LONG | RTEMS_GLOBAL)
      ) {
    #if defined(RTEMS_SMP)
	variant = SEMAPHORE_VARIANT_DFLPL;
    #else
	/*
	 * Use normal PCP on uni-processor
	 */
	variant = RTEMS_MP_NOT_CONFIGURED;
    #endif
      }else if (
	mutex_with_protocol
	  == ( RTEMS_BINARY_SEMAPHORE | RTEMS_MULTIPROCESSOR_PRIORITY_CEILING)
      ) {
    #if defined(RTEMS_SMP)
	variant = SEMAPHORE_VARIANT_MPCP;
    #else
	/*
	 * Use normal PCP on uni-processor
	 */
	variant = RTEMS_MP_NOT_CONFIGURED;
    #endif
     }else if (
	mutex_with_protocol
	  == ( RTEMS_BINARY_SEMAPHORE | RTEMS_HYPERPERIOD_DEPENDENCY_GRAPH_APPROACH | RTEMS_GLOBAL)
      ) {
    #if defined(RTEMS_SMP)
	variant = SEMAPHORE_VARIANT_HDGA;
    #else
	/*
	 * Use normal PCP on uni-processor
	 */
	variant = RTEMS_MP_NOT_CONFIGURED;
    #endif
      }else {
  return RTEMS_NOT_DEFINED;
}

  the_semaphore = _Semaphore_Allocate();

  if ( !the_semaphore ) {
    _Objects_Allocator_unlock();
    return RTEMS_TOO_MANY;
  }

#if defined(RTEMS_MULTIPROCESSING)
  the_semaphore->is_global = _Attributes_Is_global( attribute_set );

  if ( _Attributes_Is_global( attribute_set ) &&
       ! ( _Objects_MP_Allocate_and_open( &_Semaphore_Information, name,
                            the_semaphore->Object.id, false ) ) ) {
    _Semaphore_Free( the_semaphore );
    _Objects_Allocator_unlock();
    return RTEMS_TOO_MANY;
  }
#endif

  executing = _Thread_Get_executing();

  the_semaphore->variant = variant;

  if ( _Attributes_Is_priority( attribute_set ) ) {
    the_semaphore->discipline = SEMAPHORE_DISCIPLINE_PRIORITY;
  } else {
    the_semaphore->discipline = SEMAPHORE_DISCIPLINE_FIFO;
  }

  switch ( the_semaphore->variant ) {
    case SEMAPHORE_VARIANT_MUTEX_NO_PROTOCOL:
    case SEMAPHORE_VARIANT_MUTEX_INHERIT_PRIORITY:
      _CORE_recursive_mutex_Initialize(
        &the_semaphore->Core_control.Mutex.Recursive
      );

      if ( count == 0 ) {
        _CORE_mutex_Set_owner(
          &the_semaphore->Core_control.Mutex.Recursive.Mutex,
          executing
        );
        _Thread_Resource_count_increment( executing );
      }

      status = STATUS_SUCCESSFUL;
      break;
    case SEMAPHORE_VARIANT_MUTEX_PRIORITY_CEILING:
      scheduler = _Thread_Scheduler_get_home( executing );
      priority = _RTEMS_Priority_To_core( scheduler, priority_ceiling, &valid );

      if ( valid ) {
        _CORE_ceiling_mutex_Initialize(
          &the_semaphore->Core_control.Mutex,
          scheduler,
          priority
        );

        if ( count == 0 ) {
          Thread_queue_Context queue_context;

          _Thread_queue_Context_initialize( &queue_context );
          _Thread_queue_Context_clear_priority_updates( &queue_context );
          _ISR_lock_ISR_disable( &queue_context.Lock_context.Lock_context );
          _CORE_mutex_Acquire_critical(
            &the_semaphore->Core_control.Mutex.Recursive.Mutex,
            &queue_context
          );
          status = _CORE_ceiling_mutex_Set_owner(
            &the_semaphore->Core_control.Mutex,
            executing,
            &queue_context
          );

          if ( status != STATUS_SUCCESSFUL ) {
            _Thread_queue_Destroy( &the_semaphore->Core_control.Wait_queue );
          }
        } else {
          status = STATUS_SUCCESSFUL;
        }
      } else {
        status = STATUS_INVALID_PRIORITY;
      }

      break;
#if defined(RTEMS_SMP)
    case SEMAPHORE_VARIANT_MRSP:
      scheduler = _Thread_Scheduler_get_home( executing );
      priority = _RTEMS_Priority_To_core( scheduler, priority_ceiling, &valid );

      if ( valid ) {
        status = _MRSP_Initialize(
          &the_semaphore->Core_control.MRSP,
          scheduler,
          priority,
          executing,
          count == 0
        );
      } else {
        status = STATUS_INVALID_PRIORITY;
      }

      break;
    case SEMAPHORE_VARIANT_DPCP:
      scheduler = _Thread_Scheduler_get_home( executing );
      priority = _RTEMS_Priority_To_core( scheduler, priority_ceiling, &valid );

      if ( valid ) {
        status = _DPCP_Initialize(
          &the_semaphore->Core_control.DPCP,
          scheduler,
          priority,
          executing,
          count == 0
        );
      } else {
        status = STATUS_INVALID_PRIORITY;
      }

      break;
    case SEMAPHORE_VARIANT_FMLPS:
      scheduler = _Thread_Scheduler_get_home( executing );
      priority = _RTEMS_Priority_To_core( scheduler, priority_ceiling, &valid );

      if ( valid ) {
        status = _FMLPS_Initialize(
	 &the_semaphore->Core_control.FMLPS,
	 scheduler,
	 priority,
	 executing,
	 count == 0
       );
      } else {
        status = STATUS_INVALID_PRIORITY;
      }

      break;
    case SEMAPHORE_VARIANT_FMLPL:
      scheduler = _Thread_Scheduler_get_home( executing );
      priority = _RTEMS_Priority_To_core( scheduler, priority_ceiling, &valid );

      if ( valid ) {
        status = _FMLPL_Initialize(
	 &the_semaphore->Core_control.FMLPL,
	 scheduler,
	 priority,
	 executing,
	 count == 0
       );
      } else {
        status = STATUS_INVALID_PRIORITY;
      }

      break;
    case SEMAPHORE_VARIANT_DFLPL:
      scheduler = _Thread_Scheduler_get_home( executing );
      priority = _RTEMS_Priority_To_core( scheduler, priority_ceiling, &valid );

      if ( valid ) {
        status = _DFLPL_Initialize(
	 &the_semaphore->Core_control.DFLPL,
	 scheduler,
	 priority,
	 executing,
	 count == 0
       );
      } else {
        status = STATUS_INVALID_PRIORITY;
      }

      break;
    case SEMAPHORE_VARIANT_MPCP:
      scheduler = _Thread_Scheduler_get_home( executing );
      priority = _RTEMS_Priority_To_core( scheduler, priority_ceiling, &valid );

      if ( valid ) {
        status = _MPCP_Initialize(
	 &the_semaphore->Core_control.MPCP,
	 scheduler,
	 priority,
	 executing,
	 count == 0
       );
      } else {
        status = STATUS_INVALID_PRIORITY;
      }

      break;
    case SEMAPHORE_VARIANT_HDGA:
      scheduler = _Thread_Scheduler_get_home( executing );
      priority = _RTEMS_Priority_To_core( scheduler, priority_ceiling, &valid );

      if ( valid ) {
        status = _HDGA_Initialize(
	 &the_semaphore->Core_control.HDGA,
	 scheduler,
	 priority_ceiling,
	 executing,
	 count == 0
       );
      } else {
        status = STATUS_INVALID_PRIORITY;
      }

      break;
#endif
    default:
      _Assert(
        the_semaphore->variant == SEMAPHORE_VARIANT_SIMPLE_BINARY
          || the_semaphore->variant == SEMAPHORE_VARIANT_COUNTING
      );
      _CORE_semaphore_Initialize(
        &the_semaphore->Core_control.Semaphore,
        count
      );
      status = STATUS_SUCCESSFUL;
      break;
  }

  if ( status != STATUS_SUCCESSFUL ) {
    _Semaphore_Free( the_semaphore );
    _Objects_Allocator_unlock();
    return _Status_Get( status );
  }

  /*
   *  Whether we initialized it as a mutex or counting semaphore, it is
   *  now ready to be "offered" for use as a Classic API Semaphore.
   */
  _Objects_Open(
    &_Semaphore_Information,
    &the_semaphore->Object,
    (Objects_Name) name
  );

  *id = the_semaphore->Object.id;

#if defined(RTEMS_MULTIPROCESSING)
  if ( _Attributes_Is_global( attribute_set ) )
    _Semaphore_MP_Send_process_packet(
      SEMAPHORE_MP_ANNOUNCE_CREATE,
      the_semaphore->Object.id,
      name,
      0                          /* Not used */
    );
#endif
  _Objects_Allocator_unlock();
  return RTEMS_SUCCESSFUL;
}

static void _Semaphore_Manager_initialization(void)
{
  _Objects_Initialize_information( &_Semaphore_Information );

#if defined(RTEMS_MULTIPROCESSING)
  _MPCI_Register_packet_processor(
    MP_PACKET_SEMAPHORE,
    _Semaphore_MP_Process_packet
  );
#endif
}

RTEMS_SYSINIT_ITEM(
  _Semaphore_Manager_initialization,
  RTEMS_SYSINIT_CLASSIC_SEMAPHORE,
  RTEMS_SYSINIT_ORDER_MIDDLE
);
