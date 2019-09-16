/**
 * @file
 *
 * @brief RTEMS Semaphore Release
 * @ingroup ClassicSem Semaphores
 *
 * This file contains the implementation of the Classic API directive
 * rtems_semaphore_release().
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
#include <rtems/rtems/statusimpl.h>

rtems_status_code rtems_semaphore_ticket( rtems_id id, rtems_id tid, int position )
{
  Semaphore_Control    *the_semaphore;
  Thread_queue_Context  queue_context;
  ISR_lock_Context      lock_context;
  Thread_Control       *executing;
  Status_Control        status;

  executing = _Thread_Get( tid, &lock_context );
  _ISR_lock_ISR_enable( &lock_context );

  the_semaphore = _Semaphore_Get( id, &queue_context );

  if ( the_semaphore == NULL ) {
#if defined(RTEMS_MULTIPROCESSING)
    return _Semaphore_MP_Release( id );
#else
    return RTEMS_INVALID_ID;
#endif
  }

  if ( executing == NULL ) {
    return RTEMS_INVALID_ID;
  }

  _Thread_queue_Context_set_MP_callout(
    &queue_context,
    _Semaphore_Core_mutex_mp_support
  );

  switch ( the_semaphore->variant ) {
    case SEMAPHORE_VARIANT_MUTEX_INHERIT_PRIORITY:
      status =  RTEMS_NOT_DEFINED;
      break;
    case SEMAPHORE_VARIANT_MUTEX_PRIORITY_CEILING:
      status = RTEMS_NOT_DEFINED;
      break;
    case SEMAPHORE_VARIANT_MUTEX_NO_PROTOCOL:
      status = RTEMS_NOT_DEFINED;
      break;
    case SEMAPHORE_VARIANT_SIMPLE_BINARY:
      status = RTEMS_NOT_DEFINED;
      break;
#if defined(RTEMS_SMP)
    case SEMAPHORE_VARIANT_MRSP:
      status = RTEMS_NOT_DEFINED;
      break;
#endif
#if defined(RTEMS_SMP)
    case SEMAPHORE_VARIANT_DPCP:
      status = RTEMS_NOT_DEFINED;
      break;
#endif
#if defined(RTEMS_SMP)
    case SEMAPHORE_VARIANT_FMLPS:
      status = RTEMS_NOT_DEFINED;
      break;
#endif
#if defined(RTEMS_SMP)
    case SEMAPHORE_VARIANT_FMLPL:
      status = RTEMS_NOT_DEFINED;
      break;
#endif
#if defined(RTEMS_SMP)
    case SEMAPHORE_VARIANT_DFLPL:
      status = RTEMS_NOT_DEFINED;
      break;
#endif
#if defined(RTEMS_SMP)
    case SEMAPHORE_VARIANT_HDGA:
      status = _HDGA_Set_thread(
		 &the_semaphore->Core_control.HDGA,
		 executing,
		 &queue_context,
		 position
	       );
      break;
#endif

    default:
      _Assert( the_semaphore->variant == SEMAPHORE_VARIANT_COUNTING );
      status = RTEMS_NOT_DEFINED;
      break;
  }

  //return _Status_Get( status );
  return status;
}
