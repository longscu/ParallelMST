#ifndef TILE_H
#define TILE_H

#include "graph.h"

#define memory_to_use 50*1024*1024
class Tile
{
    typedef stxxl::sorter<Edge,EdgeComparator> EdgeVector;

public:
    friend class TileManager;
    friend QDebug operator<<(QDebug dbg, const Tile &tile);
    Tile (int horizontalID,int verticalID,int horizontalSize,int verticalSize,int horizontalStartPos,int verticalStartPos);
    ~Tile();


    QVector<uchar *> loadImageTile(GDALDataset* poSrcDS);
    bool computeWeight(QVector<uchar *> buffers, GDALDataset *poSrcDS);    
    void firstMerge(float minWeight);

private:
    void onComputeEdgeWeight(unsigned nodeID1, unsigned nodeID2, const QVector<double> &data1, const QVector<double> &data2);
    unsigned find(unsigned x);
    unsigned join_band_sw(unsigned x, unsigned y, float edgeWeight);



private:
    int hID;
    int vID;
    int hSize;
    int vSize;
    int hStartPos;
    int vStartPos;
    int elements;
    int num;

    EdgeVector vecEdge;     //edge weight file
    GraphElement* elts;     //graph elements
    QTemporaryFile file;
};

class TileManager
{
public:
    friend QDebug operator<<(QDebug dbg, const TileManager &tileManager);    
    TileManager(const QString &imagePath,const QString &markfilePath,int tileSize = 600);
    ~TileManager();

    void ComputeWeight();
    void FirstMerge(float minWeight);
    void MarkCrossBorderRegion();

private:
    Tile *tileAt(int i,int j);//i-row ; j-column
//    void

private:
    GDALDataset* poSrcDS;
    GDALDataset* poDstDS;
    int width,height,bandCount;
    GDALDataType type;
    int dataSize;
    int tileXCount;
    int tileYCount;
    QVector<Tile*> tiles;    
    QList<CrossBorderElement*> crossBorderLists;
};

#endif // TILE_H
