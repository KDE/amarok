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

class BlockAnalyzer : public Analyzer::Base2D
{
public:
    BlockAnalyzer( QWidget* );

    void drawAnalyzer( std::vector<float> * );

private:
    static const int BAND_COUNT=32;

    QPixmap m_block1;
    QPixmap m_block2;
};

#endif
