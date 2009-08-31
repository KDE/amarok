/****************************************************************************************
 * Copyright (c) 2004 Adam Pigg <adam@piggz.co.uk>                                      *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef GLANALYZER_H
#define GLANALYZER_H

#include <config-amarok.h>  
#ifdef HAVE_QGLWIDGET

#include "analyzerbase.h"

/**
 *@author piggz
 */

typedef struct
{
  float level;
  uint delay;
}
peak_tx;

class GLAnalyzer : public Analyzer::Base3D
{
private:
  std::vector<float> m_oldy;
  std::vector<peak_tx> m_peaks;

  void drawCube();
  void drawFrame();
  void drawBar(float xPos, float height);
  void drawPeak(float xPos, float ypos);
  void drawFloor();

  GLfloat x, y;
public:
  GLAnalyzer(QWidget *);
  ~GLAnalyzer();
  void analyze( const Scope & );
  
protected:
  void initializeGL();
  void resizeGL( int w, int h );
  void paintGL();
};

#endif
#endif
