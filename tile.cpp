#include "tile.h"
#include "stable.h"

QDebug operator<<(QDebug dbg, const Tile &tile)
{
    dbg<<QString("tile[%1,%2] \tverticalSize:%3 horizontalSize:%4 startPos:(%5,%6)")
         .arg(tile.vID).arg(tile.hID).arg(tile.vSize).arg(tile.hSize).arg(tile.hStartPos).arg(tile.vStartPos)<<"\n";
    return dbg;
}

QDebug operator<<(QDebug dbg, const TileManager &tileManager)
{
    dbg<<"width:"<< tileManager.width;
    dbg<<"height:"<< tileManager.height;
    dbg<<"bandCount:"<< tileManager.bandCount;
    dbg<<"dataSize:"<<tileManager.dataSize<<"\n";
    foreach (Tile* tile, tileManager.tiles) {
        dbg<<*tile;
    }
    return dbg;
}

Tile::Tile(int horizontalID, int verticalID, int horizontalSize, int verticalSize,int horizontalStartPos,int verticalStartPos)
    :hID(horizontalID),vID(verticalID),hSize(horizontalSize),vSize(verticalSize),
      hStartPos(horizontalStartPos),vStartPos(verticalStartPos),
      elements(horizontalSize*verticalSize),
      vecEdge(EdgeComparator(),memory_to_use),elts(NULL)
{
    num = elements;
    file.open();
    if (file.resize(elements*sizeof(GraphElement)) == false)
    {
        qDebug()<<"Resize Failed!";
        return;
    }
    elts = (GraphElement*) file.map(0, file.size());
    if (elts == NULL)
    {
        qDebug()<<"Map Failed!";
        return;
    }

    GraphElement* p = elts;
    for (unsigned i = 0; i < elements; ++i,++p)
    {
        p->rank = 0;
        p->parent = i;//每个区域（集合）的初始根节点是它本身
        p->sum_of_weight = 0;
        p->size = 1;
    }
}

Tile::~Tile()
{
    if (elts) file.unmap((uchar *)elts);
}

QVector<uchar *> Tile::loadImageTile(GDALDataset *poSrcDS)
{
    Q_ASSERT(poSrcDS);
    const int bandCount = poSrcDS->GetRasterCount();
    GDALDataType type = poSrcDS->GetRasterBand(1)->GetRasterDataType();
    int dataSize = GDALGetDataTypeSize(type)/8;
    const int &nXOff = hStartPos;
    const int &nYOff = vStartPos;
    const int &nXSize = hSize;
    const int &nYSize = vSize;

    QVector<uchar *> buffers;
    for (int k=0;k<bandCount;++k)
    {
        uchar *buffer = new uchar[nXSize*nYSize*dataSize];
        GDALRasterBand* poBand = poSrcDS->GetRasterBand(k+1);
        Q_ASSERT(poBand);
        Q_ASSERT(poBand->RasterIO(GF_Read,nXOff,nYOff,nXSize,nYSize,buffer,nXSize,nYSize,type,0,0)==CE_None);
        buffers.push_back(buffer);
    }
    return buffers;
}

bool Tile::computeWeight(QVector<uchar *> buffers,GDALDataset *poSrcDS)
{
    GDALDataType type = poSrcDS->GetRasterBand(1)->GetRasterDataType();
    int dataSize = GDALGetDataTypeSize(type)/8;
    const int bandCount = poSrcDS->GetRasterCount();
    const int &nXOff = hStartPos;
    const int &nYOff = vStartPos;
    const int &nXSize = hSize;
    const int &nYSize = vSize;

    QVector<double> buffer1(bandCount);
    QVector<double> buffer2(bandCount);

    for (unsigned y=0;y<nYSize-1;++y)
    {
        for(unsigned x=0;x<nXSize;++x)
        {
            unsigned nodeID1 = y*nXSize+x;
            unsigned nodeIDNextLIne = nodeID1+nXSize;
            if (x < nXSize-1 )
            {
                for(unsigned k=0;k<bandCount;++k)
                {
                    buffer1[k] = SRCVAL(buffers[k],type,y*nXSize+x);
                    buffer2[k] = SRCVAL(buffers[k],type,y*nXSize+x+1);
                }
                onComputeEdgeWeight(nodeID1,nodeID1+1,buffer1,buffer2);
            }
            if(y < nYSize-1)
            {
                for(unsigned k=0;k<bandCount;++k)
                {
                    buffer1[k] = SRCVAL(buffers[k],type,y*nXSize+x);
                    buffer2[k] = SRCVAL(buffers[k],type,(y+1)*nXSize+x);
                }
                onComputeEdgeWeight(nodeID1,nodeIDNextLIne,buffer1,buffer2);
            }
        }
    }

    vecEdge.sort();
    return true;
}

void Tile::firstMerge(float minWeight)
{
    while(!vecEdge.empty())
    {
        Edge edge_temp = *vecEdge;
        unsigned a = find(edge_temp.GetNode1());
        unsigned b = find(edge_temp.GetNode2());

        if (edge_temp.GetWeight()>minWeight)
            break;

        if ((a != b))
        {
            join_band_sw(a,b,edge_temp.GetWeight());
            find(a);
        }
        ++vecEdge;
    }
}

void Tile::onComputeEdgeWeight(unsigned nodeID1, unsigned nodeID2, const QVector<double> &data1, const QVector<double> &data2)
{
    double  difs=0;
    for (unsigned k=0;k<data1.size();++k)
    {
        difs += (data1[k]-data2[k])*(data1[k]-data2[k]);
    }
    vecEdge.push(Edge(nodeID1,nodeID2,(float)sqrt(difs)));
}

unsigned Tile::find(unsigned x)
{
    int y = x;
    while (y != elts[y].parent)//p为x的父节点，不相等，说明x不是根节点
        y = elts[y].parent;//找x的父节点的父节点，直到相等，说明找到了根节点
    elts[x].parent = y;//将x的父节点设为找到的根节点，优化,提高查找速度
    return y;
}

unsigned Tile::join_band_sw(unsigned x, unsigned y, float edgeWeight)
{
    if (elts[x].rank > elts[y].rank)
    {
        elts[x].size += elts[y].size; //合并所得区域的大小
        elts[x].sum_of_weight += elts[y].sum_of_weight + edgeWeight;//合并所得区域的边权和
        elts[y].parent = x;

        num--;//合并后区域数减一
        return x;
    }

    else
    {
        elts[y].size += elts[x].size;//区域大小
        elts[y].sum_of_weight += elts[x].sum_of_weight + edgeWeight;//组成该区域的边权和

        elts[x].parent = y;

        if (elts[x].rank == elts[y].rank)
            elts[y].rank++;//同时将集合y的元素数加1
        num--;
        return y;
    }
}


TileManager::TileManager(const QString &imagePath, const QString &markfilePath, int tileSize)
    :poSrcDS(NULL)
{
    GDALAllRegister();
    poSrcDS = (GDALDataset* )GDALOpen(imagePath.toUtf8().constData(),GA_ReadOnly);
    if (poSrcDS==NULL)
    {
        qWarning()<<"Open i "<<imagePath," failed!";
        return;
    }

    width = poSrcDS->GetRasterXSize();
    height = poSrcDS->GetRasterYSize();
    bandCount = poSrcDS->GetRasterCount();
    type = poSrcDS->GetRasterBand(1)->GetRasterDataType();
    dataSize = GDALGetDataTypeSize(type)/8;

    GDALDriver* poDriver = (GDALDriver*)GDALGetDriverByName("GTiff");
    Q_ASSERT(poDriver);
    GDALDataset* poDstDS = poDriver->Create(markfilePath.toUtf8().constData(),width,height,1,GDT_Int32,NULL);
    if (poDstDS == NULL)
    {
        qWarning()<<"Create markfile "<<markfilePath<<" failed!";
        return;
    }

    double adfGeoTransform[6];
    poDstDS->SetProjection(poSrcDS->GetProjectionRef());
    poSrcDS->GetGeoTransform(adfGeoTransform);
    poDstDS->SetGeoTransform(adfGeoTransform);

    // compute tile size and width,height of each tile
    tileXCount = (width+1)  / tileSize + 1;
    tileYCount = (height+1) / tileSize + 1;

    int tileXSize = width  / tileXCount;
    int tileYSize = height / tileYCount;

    int startPosX = 0;
    int startPosY = 0;
    for (int i=0;i<tileYCount;++i,startPosY += tileYSize)
    {
        startPosX=0;
        for (int j=0;j<tileXCount;++j,startPosX += tileXSize)
        {
            Tile* tile = new Tile(
                        j,i,
                        (j==(tileXCount-1))?(width -j*tileXSize):tileXSize,
                        (i==(tileYCount-1))?(height-i*tileYSize):tileYSize,
                        startPosX,startPosY);
            tiles.push_back(tile);
        }
    }
}

TileManager::~TileManager()
{
    foreach (Tile* tile, tiles) {
        if (tile) delete tile;
    }
}

void TileManager::ComputeWeight()
{
    for (int i=0;i<tileYCount;++i)
    {
        for (int j=0;j<tileXCount;++j)
        {
            Tile *tile = tileAt(i,j);
            Q_ASSERT(tile);
            QVector<uchar *> buffers = tile->loadImageTile(poSrcDS);
            tile->computeWeight(buffers,poSrcDS);
        }
    }
}

void TileManager::FirstMerge(float minWeight)
{
    for (int i=0;i<tileYCount;++i)
    {
        for (int j=0;j<tileXCount;++j)
        {
            Tile *tile = tileAt(i,j);
            Q_ASSERT(tile);
            tile->firstMerge(minWeight);
        }
    }
}

void TileManager::MarkCrossBorderRegion()
{
    //horizontal oriatation
    for (int i=0;i<tileYCount;++i)
    {
        for (int j=0;j<(tileXCount-1);++j)
        {
            Tile *tile1 = tileAt(i,j);
            Tile *tile2 = tileAt(i,j+1);
            Q_ASSERT(tile1);
            Q_ASSERT(tile2);
            Q_ASSERT(tile1->vSize == tile2->vSize);

            for (int k=0;k<tile1->vSize;++k)
            {
                int node1 = tile1->find(k*tile1->hSize+tile1->hSize-1);
                int node2 = tile2->find(k*tile2->hSize);


            }
        }
    }
}

Tile *TileManager::tileAt(int i, int j)
{
    if (i<0 || i>=tileYCount||j<0 || j>=tileXCount )
    {
        qWarning()<<QString("Out of range for visiting tile[%1,%2]!").arg(i).arg(j);
        return NULL;
    }

    return tiles.at(i*tileXCount+j);
}



