#include "digraph.h"

Digraph::Digraph()
{
}

Digraph::~Digraph()
{
    qDeleteAll(this->nodes);
}

Node* Digraph::addNode(void* userData)
{
    Node* n = new Node();
    n->userData = userData;
    this->nodes.append(n);
    return n;
}

Node* Digraph::findNodeByUserData(void *userData)
{
    Node* r = 0;
    for (int i = 0; i < this->nodes.count(); i++) {
        Node* n = this->nodes.at(i);
        if (n->userData == userData) {
            r = n;
            break;
        }
    }
    return r;
}
