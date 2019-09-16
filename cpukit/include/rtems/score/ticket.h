/**
 * @file
 *
 * @ingroup RTEMSScoreTicket
 *
 * @brief Ticket Handler API
 */


#ifndef _RTEMS_SCORE_TICKET_H
#define _RTEMS_SCORE_TICKET_H

#include <rtems/score/chain.h>
#include <rtems/score/cpu.h>
#include <rtems/score/rbtree.h>
#include <rtems/score/thread.h>

struct _Scheduler_Control;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief The ticket number of the ticket
 */
typedef uint64_t Ticket_Control;


#define TICKET_MINIMUM      0
#define TICKET_DEFAULT_MAXIMUM      255

/**
 * @brief The from RTEMS provided macro to get the container of the node
 */
#define TICKET_NODE_OF_NODE( node ) \
  RTEMS_CONTAINER_OF( node, Ticket_Node, Node.RBTree )



typedef struct {
  /**
   * @brief Node component for a chain or red-black tree.
   */
  union {
    Chain_Node Chain;
    RBTree_Node RBTree;
  } Node;

  /**
   * @brief The ticket number of the node
   */
  Ticket_Control ticket;


  /**
   * @brief The owner of this ticket
   */
  Thread_Control *owner;
} Ticket_Node;

RTEMS_INLINE_ROUTINE void _Ticket_Initialize_one(
    RBTree_Control *tree,
    Ticket_Node        *node
)
{
  _RBTree_Initialize_one( tree, &node->Node.RBTree );
}

RTEMS_INLINE_ROUTINE void _Ticket_Node_initialize(
  Ticket_Node    *node,
  Ticket_Control  ticket
)
{
  node->ticket = ticket;
  _RBTree_Initialize_node( &node->Node.RBTree );
}

/**
 * @brief Compares two tickets
 *
 * @param left The ticket control on the left hand side of the comparison.
 * @param right THe RBTree_Node to get the ticket for the comparison from.
 *
 * @retval true The ticket on the left hand side of the comparison is smaller.
 * @retval false The ticket on the left hand side of the comparison is greater of equal.
 */
RTEMS_INLINE_ROUTINE bool _Ticket_Less(
  const void        *left,
  const RBTree_Node *right
)
{
  const Ticket_Control *the_left;
  const Ticket_Node    *the_right;

  the_left = (Ticket_Control *) left;
  the_right = RTEMS_CONTAINER_OF( right, Ticket_Node, Node.RBTree );

  return *the_left < the_right->ticket;
}


/**
 * @brief Inserts ticket in to tree
 *
 * @param tree The tree to insert into
 * @param node The node that gets inserted
 * @param ticket The ticket number to compare
 *
 * @retval true The inserted node is the new minimum node according to the
 *   specified less order function.
 * @retval false The inserted node is not the new minimum node according to the
 *   specified less order function.
 */
RTEMS_INLINE_ROUTINE bool _Ticket_Plain_insert(
  RBTree_Control *tree,
  Ticket_Node        *node,
  Ticket_Control ticket
)
{
  return _RBTree_Insert_inline(
    tree,
    &node->Node.RBTree,
    &ticket,
    _Ticket_Less
  );
}



/**
 * @brief Extract the given node from the tree
 *
 * @param tree The tree to extract from
 * @param node The node that gets extracted
 *
 */
RTEMS_INLINE_ROUTINE void _Ticket_Plain_extract(
  RBTree_Control *tree,
  Ticket_Node        *node
)
{
  _RBTree_Extract( tree, &node->Node.RBTree );
}


/**
 * @brief Gets the node with the smallest ticket number from the tree
 *
 * @param tree The tree to get the minimum node from
 *
 * @return The minimum ticket node
 */
RTEMS_INLINE_ROUTINE RBTree_Node *_Ticket_Get_minimum_node(
  const RBTree_Control *tree
)
{
  return _RBTree_Minimum( tree);
}

/**
 * @brief Sets the task owner of this node
 *
 * @param node the ticket node to set the owner
 * @param executing the owner to set
 */
RTEMS_INLINE_ROUTINE void _Ticket_Set_owner(
  Ticket_Node        *node,
  Thread_Control *executing
)
{
  node->owner = executing;
}


/**
 * @brief Returns the owner of this ticket node
 *
 * @param node the ticket node to get the owner
 * @return executing the owner of this node
 */
RTEMS_INLINE_ROUTINE Thread_Control *_Ticket_Get_owner(
  Ticket_Node        *node
)
{
  return node->owner;
}


#ifdef __cplusplus
}
#endif

/** @} */

#endif
/* end of include file */
