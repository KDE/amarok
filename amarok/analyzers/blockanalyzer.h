// Maintainer: Max Howell <mac.howell@methylblue.com>, (C) 2003-4
// Copyright:  See COPYING file that comes with this distribution
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

    static const uint HEIGHT = 2;
    static const uint WIDTH  = 4;
    static const uint ROWS   = 7;

    void init();
    void transform( Scope& );
    void analyze( const Scope& );

private:
    QPixmap m_glow[ROWS];
    QPixmap m_dark;

    Scope m_store;
};

#endif
