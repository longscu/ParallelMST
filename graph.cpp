#include "graph.h"

GraphElement::GraphElement()
    :rank(0),parent(0),size(1),sum_of_weight(0),crossBorderElement(NULL)
{
}

CrossBorderElement::CrossBorderElement()
    :size(1),sum_of_weight(0)
{
}
