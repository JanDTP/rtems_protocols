/*
 *
 *  COPYRIGHT (c) 1989, 1990, 1991, 1992, 1993, 1994.
 *  On-Line Applications Research Corporation (OAR).
 *  All rights assigned to U.S. Government, 1994.
 *
 *  This material may be reproduced by or for the U.S. Government pursuant
 *  to the copyright license under the clause at DFARS 252.227-7013.  This
 *  notice must appear in all copies of this file and its derivatives.
 *
 *  $Id$
 */

#define TEST_INIT
#include "system.h"

rtems_unsigned32 time_set, eventout;

rtems_task High_tasks(
  rtems_task_argument argument
);

rtems_task Low_task(
  rtems_task_argument argument
);

void test_init();

rtems_task Init(
  rtems_task_argument argument
)
{
  rtems_status_code status;

  Print_Warning();

  puts( "\n\n*** TIME TEST 15 ***" );

  test_init();

  status = rtems_task_delete( RTEMS_SELF );
  directive_failed( status, "rtems_task_delete of RTEMS_SELF" );
}

void test_init()
{
  rtems_id          id;
  rtems_unsigned32  index;
  rtems_event_set   event_out;
  rtems_status_code status;

  time_set = FALSE;

  status = rtems_task_create(
    rtems_build_name( 'L', 'O', 'W', ' ' ),
    10,
    RTEMS_MINIMUM_STACK_SIZE,
    RTEMS_NO_PREEMPT,
    RTEMS_DEFAULT_ATTRIBUTES,
    &id
  );
  directive_failed( status, "rtems_task_create LOW" );

  status = rtems_task_start( id, Low_task, 0 );
  directive_failed( status, "rtems_task_start LOW" );

  for ( index = 1 ; index <= OPERATION_COUNT ; index++ ) {
    status = rtems_task_create(
      rtems_build_name( 'H', 'I', 'G', 'H' ),
      5,
      RTEMS_MINIMUM_STACK_SIZE,
      RTEMS_DEFAULT_MODES,
      RTEMS_DEFAULT_ATTRIBUTES,
      &Task_id[ index ]
    );
    directive_failed( status, "rtems_task_create LOOP" );

    status = rtems_task_start( Task_id[ index ], High_tasks, 0 );
    directive_failed( status, "rtems_task_start LOOP" );
  }

  Timer_initialize();
    for ( index=1 ; index <= OPERATION_COUNT ; index++ )
      (void) Empty_function();
  overhead = Read_timer();

  Timer_initialize();
    for ( index=1 ; index <= OPERATION_COUNT ; index++ )
    {
        (void) rtems_event_receive(
                 RTEMS_PENDING_EVENTS,
                 RTEMS_DEFAULT_OPTIONS,
                 RTEMS_NO_TIMEOUT,
                 &event_out
               );
    }

  end_time = Read_timer();

  put_time(
    "rtems_event_receive (current)",
    end_time,
    OPERATION_COUNT,
    overhead,
    CALLING_OVERHEAD_EVENT_RECEIVE
  );


  Timer_initialize();
    for ( index=1 ; index <= OPERATION_COUNT ; index++ )
    {
      (void) rtems_event_receive(
               RTEMS_ALL_EVENTS,
               RTEMS_NO_WAIT,
               RTEMS_NO_TIMEOUT,
               &event_out
             );
    }
  end_time = Read_timer();

  put_time(
    "rtems_event_receive (RTEMS_NO_WAIT)",
    end_time,
    OPERATION_COUNT,
    overhead,
    CALLING_OVERHEAD_EVENT_RECEIVE
  );
}

rtems_task Low_task(
  rtems_task_argument argument
)
{
  rtems_unsigned32  index;
  rtems_event_set   event_out;

  end_time = Read_timer();

  put_time(
    "rtems_event_receive (blocking)",
    end_time,
    OPERATION_COUNT,
    0,
    CALLING_OVERHEAD_EVENT_RECEIVE
  );

  Timer_initialize();
    for ( index=1 ; index <= OPERATION_COUNT ; index++ )
      (void) Empty_function();
  overhead = Read_timer();

  Timer_initialize();
    for ( index=1 ; index <= OPERATION_COUNT ; index++ )
      (void) rtems_event_send( RTEMS_SELF, RTEMS_EVENT_16 );
  end_time = Read_timer();

  put_time(
    "rtems_event_send (non-blocking)",
    end_time,
    OPERATION_COUNT,
    overhead,
    CALLING_OVERHEAD_EVENT_SEND
  );

  Timer_initialize();
    (void) rtems_event_receive(
             RTEMS_EVENT_16,
             RTEMS_DEFAULT_OPTIONS,
             RTEMS_NO_TIMEOUT,
             &event_out
           );
  end_time = Read_timer();

  put_time(
    "rtems_event_receive (available)",
    end_time,
    1,
    0,
    CALLING_OVERHEAD_EVENT_RECEIVE
  );

  Timer_initialize();
    for ( index=1 ; index <= OPERATION_COUNT ; index++ )
      (void) rtems_event_send( Task_id[ index ], RTEMS_EVENT_16 );
  end_time = Read_timer();

  put_time(
    "rtems_event_send (readying)",
    end_time,
    OPERATION_COUNT,
    overhead,
    CALLING_OVERHEAD_EVENT_SEND
  );

  puts( "*** END OF TEST 15 ***" );
  exit( 0 );
}

rtems_task High_tasks(
  rtems_task_argument argument
)
{
  rtems_status_code status;

  if ( time_set )
    status = rtems_event_receive(
      RTEMS_EVENT_16,
      RTEMS_DEFAULT_OPTIONS,
      RTEMS_NO_TIMEOUT,
      &eventout
    );
  else {
    time_set = 1;
    Timer_initialize();            /* start blocking rtems_event_receive time */
    status = rtems_event_receive(
      RTEMS_EVENT_16,
      RTEMS_DEFAULT_OPTIONS,
      RTEMS_NO_TIMEOUT,
      &eventout
    );
  }
}
