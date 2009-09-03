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

#include <config-amarok.h>  
#ifdef HAVE_QGLWIDGET

#ifndef GLANALYZER3_H
#define GLANALYZER3_H

#include "analyzerbase.h"
#include <QList>
#include <QString>

class QWidget;
class Ball;
class Paddle;

class GLAnalyzer3 : public Analyzer::Base3D
{
public:
    GLAnalyzer3(QWidget *);
    ~GLAnalyzer3();
    void analyze( const Scope & );
    void paused();

protected:
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

private:
    struct ShowProperties {
	double timeStamp;
	double dT;
	float colorK;
	float gridScrollK;
	float gridEnergyK;
	float camRot;
	float camRoll;
	float peakEnergy;
    } show;

    struct FrameProperties {
	bool silence;
	float energy;
	float dEnergy;
    } frame;
    
    static const int NUMBER_OF_BALLS = 16;
    
    QList<Ball*> balls;
    Paddle * leftPaddle, * rightPaddle;
    float unitX, unitY;
    GLuint ballTexture;
    GLuint gridTexture;

    void drawDot3s( float x, float y, float z, float size );
    void drawHFace( float y );
    void drawScrollGrid( float scroll, float color[4] );

    bool loadTexture(QString file, GLuint& textureID);
    void freeTexture(GLuint& textureID);
};

#endif
#endif
