/***************************************************************************
                          baranalyzer2.h  -  description
                             -------------------
    begin                : Die Jan 7 2003
    copyright            : (C) 2003 by Adam Pigg
    email                :
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef VISWIDGETV2_H
#define VISWIDGETV2_H

#include "analyzerbase.h"

#include <vector>

class QMouseEvent;
class QPixmap;
class QWidget;

#undef  BAND_COUNT
#define BAND_COUNT 32

/**
 *@author piggz
 */

typedef struct
{
  unsigned char level;
  unsigned char delay;
}
peak_t;

class BarAnalyzer2 : public Base2D
{
  Q_OBJECT

public:
  BarAnalyzer2(QWidget *parent=0, const char *name=0);
  virtual ~BarAnalyzer2();

  virtual void drawAnalyzer( std::vector<float> * );

protected:
  virtual void init();
  std::vector<float> demoData();

  QPixmap *m_pBgPixmap;
  QPixmap *m_pSrcPixmap;
  QPixmap *m_pComposePixmap;
  QPixmap *m_pRoofPixmap;

  // ATTRIBUTES:
  std::vector<peak_t> m_peakArray;
  std::vector<uint> m_barArray;;
  std::vector<uint> m_lvlMap;
  std::vector<float> m_bands;
  //std::vector<float> m_freqMap; //See .cpp file init() for description
};
#endif
