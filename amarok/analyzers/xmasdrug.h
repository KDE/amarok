/***************************************************************************
                          xmasdrug.h  -  Special Xmas analyzer
                             -------------------
    begin                : Dec 20 2003
    copyright            : (C) 2003 by Stanislav Karchebny
    email                : berk@upnet.ru
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef ANALYZER_XMASDRUG_H
#define ANALYZER_XMASDRUG_H

#include "analyzerbase.h"

class QBitmap;

/**
@author Stanislav Karchebny
*/

class XmasAnalyzer : public Analyzer::Base2D
{
   public:
      XmasAnalyzer( QWidget * );
      ~XmasAnalyzer();

      void init();
      void analyze( const Scope & );

   protected:
      void drawStar( QPainter &p, int x, int y, QColor startColor );

      static const uint BAND_COUNT=7;

      QBitmap     *m_pBuckPixmap;
      QPixmap     *m_pSantaPixmap;

      uint m_levelToX[256];
};

#endif
