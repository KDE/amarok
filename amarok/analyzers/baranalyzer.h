// Maintainer: Max Howell <max.howell@methylblue.com>
// Authors:    Mark Kretcshmann & Max Howell (C) 2003-4
// Copyright:  See COPYING file that comes with this distribution
//

#ifndef BARANALYZER_H
#define BARANALYZER_H

#include "analyzerbase.h"
//#include <qvaluevector.h> //stack allocated

/**
@author Mark Kretschmann && Max Howell
*/

using Analyzer::Scope;

class BarAnalyzer : public Analyzer::Base2D
{
    public:
        BarAnalyzer( QWidget* );

        void init();
        virtual void analyze( const Scope& );

        static const uint BAND_COUNT = 32;
        static const uint ROOF_HOLD_TIME = 48;
        static const int  ROOF_VELOCITY_REDUCTION_FACTOR = 32;
        static const uint NUM_ROOFS = 16;
        static const uint COLUMN_WIDTH = 4;

    protected:
        QPixmap m_pixRoof[NUM_ROOFS];
        std::vector<uint> m_roofMem[BAND_COUNT];

        Scope m_bands; //copy of the Scope to prevent creating/destroying a Scope every iteration
        uint  m_lvlMapper[256];

        std::vector<uint> barVector;          //positions of bars
        std::vector<int>  roofVector;         //positions of roofs
        std::vector<uint> roofVelocityVector; //speed that roofs falls

        const QPixmap *gradient() const { return &m_pixBarGradient; }

    private:
        QPixmap m_pixBarGradient;
        QPixmap m_pixCompose;
};

#endif
