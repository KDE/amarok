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

#include "DiscoAnalyzer.h"

#include <QImage>
#include <QStandardPaths>

#include <cmath>
#include <cstdlib>
#include <sys/time.h>


DiscoAnalyzer::DiscoAnalyzer( QWidget *parent ):
    Analyzer::Base( parent )
{
    setObjectName( "Disco" );

    m_dotTexture = bindTexture( QImage( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/dot.png" ) ) );
    m_w1Texture = bindTexture( QImage( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/wirl1.png" ) ) );
    m_w2Texture = bindTexture( QImage( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/wirl2.png" ) ) );

    m_show.paused = true;
    m_show.pauseTimer = 0.0;
    m_show.rotDegrees = 0.0;
    m_frame.rotDegrees = 0.0;
}

DiscoAnalyzer::~DiscoAnalyzer()
{
    deleteTexture( m_dotTexture );
    deleteTexture( m_w1Texture );
    deleteTexture( m_w2Texture );
}

void DiscoAnalyzer::demo()
{
    static int t = 0;
    static bool forward = true;

    QVector<float> s( 200 );
    const double dt = double( t ) / 200;

    for( int i = 0; i < s.size(); ++i )
        s[i] = dt * ( sin( M_PI + ( i * M_PI ) / s.size() ) + 1.0 );

    analyze( s );

    if( t == 0 )
        forward = true;
    if( t == 200 )
        forward = false;

    t = forward ? t + 2 : t - 2;
}

void DiscoAnalyzer::initializeGL()
{
    // Set a smooth shade model
    glShadeModel( GL_SMOOTH );

    // Disable depth test (all is drawn on a 2d plane)
    glDisable( GL_DEPTH_TEST );

    // Set blend parameters for 'composting alpha'
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );                        //superpose
    //glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );        //fade
    glEnable( GL_BLEND );

    // Clear frame with a black background
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
    glClear( GL_COLOR_BUFFER_BIT );
}

void DiscoAnalyzer::resizeGL( int w, int h )
{
    // Setup screen. We're going to manually do the perspective projection
    glViewport( 0, 0, ( GLint )w, ( GLint )h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glOrtho( -10.0f, 10.0f, -10.0f, 10.0f, -5.0f, 5.0f );

    // Get the aspect ratio of the screen to draw 'cicular' particles
    const float ratio = ( float )w / ( float )h,
          eqPixH = 60,
          eqPixW = 80;
    if( ratio >= ( 4.0 / 3.0 ) )
    {
        m_unitX = 10.0 / ( eqPixH * ratio );
        m_unitY = 10.0 / eqPixH;
    }
    else
    {
        m_unitX = 10.0 / eqPixW;
        m_unitY = 10.0 / ( eqPixW / ratio );
    }

    // Get current timestamp.
    timeval tv;
    gettimeofday( &tv, NULL );
    m_show.timeStamp = ( double )tv.tv_sec + ( double )tv.tv_usec / 1000000.0;
}

void DiscoAnalyzer::analyze( const QVector<float> &s )
{
    bool haveNoData = s.empty();

    // if we're going into pause mode, clear timers.
    if( !m_show.paused && haveNoData )
        m_show.pauseTimer = 0.0;

    // if we have got data, interpolate it (asking myself why I'm doing it here..)
    if( !( m_show.paused = haveNoData ) )
    {
        int bands = s.size(),
            lowbands = bands / 4,
            hibands = bands / 3,
            midbands = bands - lowbands - hibands; Q_UNUSED( midbands );
        float currentEnergy = 0,
              currentMeanBand = 0,
              maxValue = 0;
        for( int i = 0; i < bands; i++ )
        {
            float value = s[i];
            currentEnergy += value;
            currentMeanBand += ( float )i * value;
            if( value > maxValue )
                maxValue = value;
        }
        m_frame.silence = currentEnergy < 0.001;
        if( !m_frame.silence )
        {
            m_frame.meanBand = 100.0 * currentMeanBand / ( currentEnergy * bands );
            currentEnergy = 100.0 * currentEnergy / ( float )bands;
            m_frame.dEnergy = currentEnergy - m_frame.energy;
            m_frame.energy = currentEnergy;
//            printf( "%d  [%f :: %f ]\t%f \n", bands, frame.energy, frame.meanBand, maxValue         );
        }
        else
            m_frame.energy = 0.0;
    }
}

void DiscoAnalyzer::paintGL()
{

}

void DiscoAnalyzer::drawDot( float x, float y, float size )
{
    float sizeX = size * m_unitX,
          sizeY = size * m_unitY,
          pLeft = x - sizeX,
          pTop = y + sizeY,
          pRight = x + sizeX,
          pBottom = y - sizeY;
    glTexCoord2f( 0, 0 );        // Bottom Left
    glVertex2f( pLeft, pBottom );
    glTexCoord2f( 0, 1 );        // Top Left
    glVertex2f( pLeft, pTop );
    glTexCoord2f( 1, 1 );        // Top Right
    glVertex2f( pRight, pTop );
    glTexCoord2f( 1, 0 );        // Bottom Right
    glVertex2f( pRight, pBottom );
}

void DiscoAnalyzer::drawFullDot( float r, float g, float b, float a )
{
    glBindTexture( GL_TEXTURE_2D, m_dotTexture );
    glEnable( GL_TEXTURE_2D );
    glColor4f( r, g, b, a );
    glBegin( GL_TRIANGLE_STRIP );
    glTexCoord2f( 1.0, 1.0 );
    glVertex2f( 10.0f, 10.0f );
    glTexCoord2f( 0.0, 1.0 );
    glVertex2f( -10.0f, 10.0f );
    glTexCoord2f( 1.0, 0.0 );
    glVertex2f( 10.0f, -10.0f );
    glTexCoord2f( 0.0 , 0.0 );
    glVertex2f( -10.0f, -10.0f );
    glEnd();
    glDisable( GL_TEXTURE_2D );
}


void DiscoAnalyzer::setTextureMatrix( float rot, float scale )
{
    glMatrixMode( GL_TEXTURE );
    glLoadIdentity();
    if( rot != 0.0 || scale != 0.0 )
    {
        glTranslatef( 0.5f, 0.5f, 0.0f );
        glRotatef( rot, 0.0f, 0.0f, 1.0f );
        glScalef( scale, scale, 1.0f );
        glTranslatef( -0.5f, -0.5f, 0.0f );
    }
    glMatrixMode( GL_MODELVIEW );
}

