#pragma once
#include <QThread>
#include "Src\TypeTree.h"

class AnalysisPEFileThread : public QThread
{
    TypeTree    *mStructTree;
public:
    AnalysisPEFileThread( TypeTre *structTree );
    ~AnalysisPEFileThread( );

    void run( );
};

