//
// Amarok BarAnalyzer 3 - Jet Turbine: Symmetric version of analyzer 1
//
// Author: Stanislav Karchebny <berkus@users.sf.net>, (C) 2003
//
// Copyright: like rest of amaroK
//

#ifndef ANALYZER_TURBINE_H
#define ANALYZER_TURBINE_H

#include "baranalyzer.h"

class TurbineAnalyzer : public BarAnalyzer
{
    public:
        TurbineAnalyzer( QWidget *parent ) : BarAnalyzer( parent ), m_roofPixmap( 4, 1 ) { m_roofPixmap.fill( 0xff5070 ); };

        void drawAnalyzer( std::vector<float> * );

    private:
        QPixmap m_roofPixmap;
};

#endif
