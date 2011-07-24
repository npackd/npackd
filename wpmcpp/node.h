#ifndef NODE_H
#define NODE_H

#include <qlist.h>

/**
 * A digraph node.
 */
class Node
{
public:
    /**
     * Edges going from this node to the nodes in "to". If this Node represents
     * a package version, "to" would represent dependencies. The objects in this
     * list will *not* be freed if this Node is freed.
     */
    QList<Node*> to;

    /** anything can be stored here. Initialized with 0. */
    void* userData;

    /** Constructs a node without edges */
    Node();
};

#endif // NODE_H
