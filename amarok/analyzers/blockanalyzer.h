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

    void init();
    void transform( Scope& );
    void analyze( const Scope& );

private:
    QPixmap m_glow;
    QPixmap m_dark;

    Scope m_store;
};

#endif
