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


class QPixmap;

/**
@author Mark Kretschmann
*/

class BarAnalyzer : public AnalyzerBase
{
    Q_OBJECT

    public:
        BarAnalyzer( QWidget *parent=0, const char *name=0 );
        virtual ~BarAnalyzer();

        virtual void init();
        virtual void drawAnalyzer( std::vector<float> * );

    protected:
        QPixmap *m_pBgPixmap;
        QPixmap *m_pSrcPixmap;
        QPixmap *m_pComposePixmap;
        QPixmap *m_pRoofPixmap;

        uint m_lvlMapper[256];
};

#endif
