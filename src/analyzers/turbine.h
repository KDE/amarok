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
        TurbineAnalyzer( QWidget *parent ) : BarAnalyzer( parent ) {}

        void analyze( const Scope& );
};

#endif
