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


class QPixmap;

/**
@author Stanislav Karchebny
*/

class XmasAnalyzer : public AnalyzerBase
{
   Q_OBJECT

   public:
      XmasAnalyzer( QWidget *parent=0, const char *name=0 );
      virtual ~XmasAnalyzer();

      virtual void init();
      virtual void drawAnalyzer( std::vector<float> * );

   protected:
      QPixmap     *m_pComposePixmap;
      QBitmap     *m_pBuckPixmap;
      QPixmap     *m_pSantaPixmap;

      uint m_levelToX[256];
};

#endif
