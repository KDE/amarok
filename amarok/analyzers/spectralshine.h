/***************************************************************************
                          spectralshine.h  -  SpectralShine analyzer
                             -------------------
    begin                : Dec 4 2003
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

#ifndef ANALYZER_SPECTRALSHINE_H
#define ANALYZER_SPECTRALSHINE_H

#include "analyzerbase.h"


/**
@author Stanislav Karchebny
*/

class SpectralShineAnalyzer : public Analyzer::Base2D
{
   Q_OBJECT

   public:
      typedef QValueList<QPixmap *> PixmapList;

      SpectralShineAnalyzer( QWidget * );
      ~SpectralShineAnalyzer();

      void init();
      void analyze( const Scope & );

   protected:
      QPixmap     *m_pSrcPixmap;
      QPixmap     *m_pGradient;

      uint m_levelToX[256], m_levelToY[256];
};

#endif
