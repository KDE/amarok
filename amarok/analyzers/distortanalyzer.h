/***************************************************************************
                          visdistortwidget.h  -  description
                             -------------------
    begin                : Oct 27 2003
    copyright            : (C) 2003 by Mark Kretschmann
    email                : markey@web.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef VISDISTORTWIDGET_H
#define VISDISTORTWIDGET_H

#include "analyzerbase.h"
#include <vector>

class QPixmap;

class DistortAnalyzer : public Analyzer::Base2D
{
    public:
        DistortAnalyzer( QWidget * );
        ~DistortAnalyzer();

    private:
        void init();
        void drawAnalyzer( std::vector<float> * );
        inline int checkIndex( int, int );

    // ATTRIBUTES:
        QPixmap *m_pComposePixmap1;
        std::vector<float> m_sinVector;
        std::vector<QPixmap*> m_srcPixmaps;
};

#endif
