//
// C++ Interface: blockanalyzer
//
// Description:
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BLOCKANALYZER_H
#define BLOCKANALYZER_H

#include "analyzerbase.h"

/**
@author Max Howell
*/

class BlockAnalyzer : public AnalyzerBase2d
{
Q_OBJECT

public:
    BlockAnalyzer( QWidget * = 0, const char * = 0 );
    ~BlockAnalyzer();

    virtual void init();
    virtual void drawAnalyzer( std::vector<float> * );

private:
    QPixmap m_block1;
    QPixmap m_block2;
};

#endif
