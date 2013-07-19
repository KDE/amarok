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

#ifndef BALLS_ANALYZER_H
#define BALLS_ANALYZER_H

#include "AnalyzerBase.h"


class QWidget;
class Ball;
class Paddle;

class BallsAnalyzer : public Analyzer::Base
{
public:
    BallsAnalyzer( QWidget * );
    ~BallsAnalyzer();
    void analyze( const QVector<float> & );

protected:
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

private:
    struct ShowProperties
    {
        double timeStamp;
        double dT;
        float colorK;
        float gridScrollK;
        float gridEnergyK;
        float camRot;
        float camRoll;
        float peakEnergy;
    } m_show;

    struct FrameProperties
    {
        bool silence;
        float energy;
        float dEnergy;
    } m_frame;

    static const int NUMBER_OF_BALLS = 16;

    QList<Ball *> m_balls;
    Paddle * m_leftPaddle, * m_rightPaddle;
    float m_unitX, m_unitY;
    GLuint m_ballTexture;
    GLuint m_gridTexture;

    void drawDot3s( float x, float y, float z, float size );
    void drawHFace( float y );
    void drawScrollGrid( float scroll, float color[4] );
};

#endif
