//
//
// C++ Interface: $MODULE$
//
// Description:
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef BARANALYZER_H
#define BARANALYZER_H

#include "analyzerbase.h"

#include <qvaluevector.h>

//we undef so --enable-final works
#undef BAND_COUNT
#undef ROOF_HOLD_TIME
#undef ROOF_VELOCITY_REDUCTION_FACTOR
#undef MAX_AMPLITUDE
#undef NUM_ROOFS

#define BAND_COUNT 32
#define ROOF_HOLD_TIME 48
#define ROOF_VELOCITY_REDUCTION_FACTOR 32
#define MAX_AMPLITUDE 1.0
#define NUM_ROOFS 16

/**
@author Mark Kretschmann && Max Howell
*/

class BarAnalyzer : public AnalyzerBase2d
{
    Q_OBJECT

    public:
        BarAnalyzer( QWidget *parent=0, const char *name=0 );
        virtual ~BarAnalyzer();

        virtual void init();
        virtual void drawAnalyzer( std::vector<float> * );

    protected:
        QPixmap *m_pSrcPixmap;
        QPixmap *m_pComposePixmap;
        QPixmap  m_roofPixmaps[ NUM_ROOFS ];
        QValueVector<int> m_roofMem[ BAND_COUNT ];

        //FIXME
        QPixmap m_roofPixmap;

        uint m_lvlMapper[256];
};

#endif
