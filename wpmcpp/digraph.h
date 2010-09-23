#ifndef DIGRAPH_H
#define DIGRAPH_H

#include <qlist.h>

#include "node.h"

/**
 * A directed graph of Node
 */
class Digraph
{
public:
    /** all the nodes. The objects will be released. */
    QList<Node*> nodes;

    /** creates an empty digraph */
    Digraph();

    virtual ~Digraph();

    /**
     * Creates a new node without edges and adds it to this digraph.
     *
     * @param userData will be stored in Node.userData
     * @return created node
     */
    Node* addNode(void* userData);

    /**
     * Searches for a node with the specified user data.
     *
     * @return the found node or 0
     */
    Node* findNodeByUserData(void* userData);
};

#endif // DIGRAPH_H
