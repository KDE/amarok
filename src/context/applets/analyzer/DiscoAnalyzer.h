/****************************************************************************************
 * Copyright (c) 2004 Enrico Ros <eros.kde@email.it>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DISCO_ANALYZER_H
#define DISCO_ANALYZER_H


#include "AnalyzerBase.h"


class QPaintEvent;

class DiscoAnalyzer : public Analyzer::Base
{
public:
    DiscoAnalyzer( QWidget * );
    ~DiscoAnalyzer();
    void analyze( const QVector<float>& );

protected:
    void demo();
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

private:
    struct ShowProperties
    {
        bool paused;
        double timeStamp;
        double dT;
        double pauseTimer;
        float rotDegrees;
    } m_show;

    struct FrameProperties
    {
        float energy;
        float dEnergy;
        float meanBand;
        float rotDegrees;
        bool silence;
    } m_frame;

    GLuint m_dotTexture;
    GLuint m_w1Texture;
    GLuint m_w2Texture;
    float m_unitX, m_unitY;

    void drawDot( float x, float y, float size );
    void drawFullDot( float r, float g, float b, float a );
    void setTextureMatrix( float rot, float scale );
};

#endif
