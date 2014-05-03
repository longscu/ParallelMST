#include <QCoreApplication>
#include "stable.h"
#include "tile.h"

int main(int argc, char *argv[])
{
    if (argc>2)
    {
        TileManager tileManager(QString::fromLocal8Bit( argv[1]),QString::fromLocal8Bit( argv[2]));
        tileManager.ComputeWeight();
        tileManager.FirstMerge(25);
    }

    return 0;
}
