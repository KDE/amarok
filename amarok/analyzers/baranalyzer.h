//
// Author: Mark Kretschmann <markey@web.de>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef BARANALYZER_H
#define BARANALYZER_H

#include "analyzerbase.h"
#include <qvaluevector.h> //stack allocated

/**
@author Mark Kretschmann && Max Howell
*/

class BarAnalyzer : public Analyzer::Base2D
{
    public:
        BarAnalyzer( QWidget* );

        virtual void init();
        virtual void drawAnalyzer( std::vector<float> * );

        static const uint BAND_COUNT=32;
        static const uint ROOF_HOLD_TIME=48;
        static const int  ROOF_VELOCITY_REDUCTION_FACTOR = 32;
        static const uint NUM_ROOFS=16;

    protected:
        QPixmap m_roofPixmaps[ NUM_ROOFS ];
        QValueVector<int> m_roofMem[ BAND_COUNT ];

        std::vector<float> m_bands;

        uint m_lvlMapper[256];

        const QPixmap *gradient() const { return &m_gradientPixmap; }

    private:
        QPixmap  m_gradientPixmap;
        QPixmap  m_composePixmap;
};

#endif
