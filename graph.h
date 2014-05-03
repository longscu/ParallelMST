#ifndef GRAPH_H
#define GRAPH_H

class Edge
{
public:
    Edge(){}
    Edge(unsigned nodeID1,unsigned nodeID2,float weight):_weight(weight),_nodeID1(nodeID1),_nodeID2(nodeID2){}
    bool operator<(const Edge& other) const
    {
        return _weight<other._weight;
    }
    float GetWeight()const{return _weight;}
    unsigned GetNode1()const{return _nodeID1;}
    unsigned GetNode2()const{return _nodeID2;}
private:
    float _weight;
    unsigned _nodeID1;
    unsigned _nodeID2;
};

struct EdgeComparator
{
    bool operator () (const Edge& a, const Edge& b) const
    {
        return a.GetWeight() < b.GetWeight();
    }
    Edge min_value() const
    {
        return Edge(0,0,std::numeric_limits<float>::min());
    }
    Edge max_value() const
    {
        return Edge(0,0,std::numeric_limits<float>::max());
    }
};

class GraphElement
{
public:
    friend class Tile;
    friend class TileManager;
    GraphElement();

private:
    int rank;
    int parent;
    int size;
    float sum_of_weight;
    CrossBorderElement* crossBorderElement;
};

class CrossBorderElement
{
public:
    friend class Tile;
    CrossBorderElement();

private:
    int size;
    float sum_of_weight;
};

#endif // GRAPH_H
