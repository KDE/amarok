/****************************************************************************************
 * Copyright (c) 2010 Daniel Dewald <Daniel.Dewald@time-shift.de>                       *
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

#include "AnalyzerGlWidget.h"
#include "SplineInterpolation.h"

AnalyzerGlWidget::AnalyzerGlWidget( QGLContext *glContext, QColor fillColor )
    :QGLWidget( glContext )
    ,m_fillColor( fillColor )
    ,m_mode( Bars )
    ,m_showPeaks( false )
    ,m_showWave( false )
    ,m_peakSinkRate( 1.0f )    
    ,m_barsPerDot( 15 )
{
    changecount = 100;
}

AnalyzerGlWidget::~AnalyzerGlWidget()
{

}

void AnalyzerGlWidget::initializeGLScene()
{
    // If you wonder where the values came from: trial and error
    glClearColor( m_fillColor.redF() * 1.2, m_fillColor.greenF() * 1.077, m_fillColor.blueF(), m_fillColor.alphaF() * 0.5 );
    glDisable( GL_DEPTH_TEST );
    glEnable( GL_CULL_FACE );
    glEnable( GL_BLEND );
    glEnable( GL_TEXTURE_2D );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glHint( GL_POLYGON_SMOOTH_HINT, GL_NICEST );
    glHint( GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST );
}

void AnalyzerGlWidget::resizeGL(int width, int height)
{
    resizeGLScene(width, height);
}

void AnalyzerGlWidget::resizeGLScene( int width, int height )
{
    glViewport( 0, 0, (GLint)width, (GLint)height );
}

void AnalyzerGlWidget::paintGL()
{
    paintGLScene();
}

void AnalyzerGlWidget::paintBars( QVector<int> values )
{
    if( m_peaks.size() < values.size() )
        m_peaks = values;
    
    float barWidth = ( 2.0f / values.size() ) * 9 / 10;
    float barSpacer = barWidth / 10;

    glTranslatef( -1.0f + ( barSpacer / 2 ) , 0.0f, 0.0f );

    for ( int x = 0; x < values.size(); x++ )
    {
        float strength = 0.0077f * values[x];
        glTranslatef( barSpacer, 0.0f, 0.0f );

        glBegin( GL_TRIANGLE_STRIP );
           glColor4f( 0.0f, 1.0f, 0.0f, 0.7f );
           glVertex3d(  barWidth, -1.0f, -0.1f );
           glColor4f( strength, 0.3f / strength, 0.0f, 0.7f );
           glVertex3d(  barWidth,  strength - 1.0f, -0.1f );
           glColor4f( 0.0f, 1.0f, 0.0f, 0.7f );
           glVertex3d( 0.0f, -1.0f, -0.1f );
           glColor4f( strength, 0.3f / strength, 0.0f, 0.7f );
           glVertex3d( 0.0f,  strength - 1.0f, -0.1f );
        glEnd();

        if( m_showPeaks )
        {
            if( m_peaks[x] > values[x] )
            {
                m_peaks[x] -= m_peakSinkRate;
            }
            else
            {
                m_peaks[x] = values[x];
            }

            float peakPosition = 0.0077f * m_peaks[x];
            float peakHeight = barWidth / 2;

            glBegin( GL_TRIANGLE_STRIP );
                glColor4f( 0.0f, 0.0f , 1.0f, 0.8f );
                glVertex3d(  barWidth, peakPosition - 0.97f, -0.1f );
                glColor4f( 0.0f, 0.0f , 1.0f, 0.8f );
                glVertex3d(  barWidth,  peakPosition - 0.97f + peakHeight, -0.1f );
                glColor4f( 0.0f, 0.0f , 1.0f, 0.8f );
                glVertex3d( 0.0f, peakPosition - 0.97f, -0.1f );
                glColor4f( 0.0f, 0.0f , 1.0f, 0.8f );
                glVertex3d( 0.0f,  peakPosition - 0.97f + peakHeight, -0.1f );
            glEnd();
        }

        glTranslatef( barWidth, 0.0f, 0.0f );
    }

    if( m_showWave )
    {
        glLoadIdentity();
        paintWave( values );
    }
}

QVector<int> AnalyzerGlWidget::interpolateSpline( QVector<int> values, int size )
{
    if( ( values.size() < 1 ) || ( size <= values.size() ) )
        return values;

    QVector<double> pointsx;
    QVector<double> pointsy;

    for( int y = 0; y < values.size(); y++ )
    {
        pointsx.append( size / values.size() * y );
        pointsy.append( values[y] );
    }

    pointsx.append( size );
    pointsy.append( values[values.size()-1] );

    QVector<int> result;
    QVector<double> b;
    QVector<double> c;
    QVector<double> d;

    b.fill( 0.0, pointsx.size() );
    c.fill( 0.0, pointsx.size() );
    d.fill( 0.0, pointsx.size() );
    cubicNak( pointsx, pointsy, b, c, d );

    for( int x = 0; x < size; x++ )
    {
        result.append( splineEval( pointsx, pointsy, b, c, d, x ) );
    }

    return result;
}

void AnalyzerGlWidget::paintWave( QVector<int> values )
{
    QVector<int> interpolated = interpolateSpline( values, 500 );

    glColor4f( 0.0f, 0.0f, 1.0f, 0.7f );
    glBegin( GL_LINE_STRIP );
        for( int x = 0; x < 500; x++ )
        {
            glVertex2f( 2.0f / interpolated.size() * x - 0.99f, 0.0077f * interpolated[x] - 1.0f );
        }
        glVertex2f( 1.0f, 0.0077f * interpolated[interpolated.size()-1] - 1.0f );
    glEnd();
}

void AnalyzerGlWidget::paintWaterfall( QVector<int> values )
{
    if( m_lastValues.size() > 512 )
        m_lastValues.removeAt( 0 );

    m_lastValues.append( values );

    for( int x = 0; x < m_lastValues.size(); x++ )
    {
        for( int y = 0; y < m_lastValues.value( 0 ).size(); y++ )
        {
            //TODO: Paint field with colors
            glBegin( GL_TRIANGLE_STRIP );
                glVertex3d( 1.0f, -1.0, -0.1 );
                glVertex3d( 1.0f,  1.0, -0.1 );
                glVertex3d( -1.0, -1.0, -0.1 );
                glVertex3d( -1.0,  1.0, -0.1 );
            glEnd();
        }
    }
}

void AnalyzerGlWidget::paint3DWaves( QVector<int> values )
{
    if( m_lastValues.size() > 255 )
        m_lastValues.removeAt( 0 );
    
    m_lastValues.append( values );
    GLfloat red, green, blue;
    int frequencys = values.size();
    float frequencyStep = 2.0f / frequencys;

    red = 1.0f;
    green = 0.0f;
    blue = 0.0f;

    glRotatef( 15.0f, 2.0f, 1.0f, 0.0f );
    for( int x = 0; x < frequencys; x++ )
    {
        glColor3f( red, green, blue );
        glBegin( GL_LINE_STRIP );
            for( int y = 0; y < m_lastValues.size(); y++ )
            {
                glVertex3f( ( 1.5f / frequencys ) * x - 0.75f, 0.0078f * m_lastValues[y][x] - 1.0f, ( 2.0f / m_lastValues.size() ) * y - 1.0f );
            }
            glVertex3f( ( 1.5f / frequencys ) * x - 0.75f, -0.5f, 1.0f );
            glVertex3f( ( 1.5f / frequencys ) * x - 0.75f, -0.5f, 1.1f );
        glEnd();

        if( red > 0.0f )
        {
            red -= frequencyStep;
            green += frequencyStep;
        }
        else if ( blue < 1.0f )
        {
            green -= frequencyStep;
            blue += frequencyStep;
        }
    }
}

void AnalyzerGlWidget::paint3DChannels( QMap<int,QVector<int> > values )
{
    int valueSize = values.value( 0 ).size();
    
    for( int x = 0; x < valueSize; x++ )
    {
        
    }
}

void AnalyzerGlWidget::paintGLScene()
{
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glLoadIdentity();

    if( changecount > 5 )
    {
        m_frequencyValues.clear();
        for ( int x = 0; x < 256; x++ )
        {
            m_frequencyValues.append( rand() % 255 );
        }
        changecount = 0;
    }
    else
    {
        changecount++;
    }

    if( m_frequencyValues.size() < 1 )
        return;

    QVector<int> interpolatedValues;

    if( m_mode != Channels3D )
    {
        int numBars = NUMDOTS * m_barsPerDot;

        if ( numBars > m_frequencyValues.size() )
        {
            numBars = m_frequencyValues.size();
        }
    
        int valuesPerBar = m_frequencyValues.size() / numBars;

        for( int y = 0; y < numBars; y++ )
        {
            int buffer = 0.0f;

            for( int z = 0; z < valuesPerBar; z++ )
            {
                buffer += abs( m_frequencyValues[y+z] );
            }

            interpolatedValues.append( buffer / valuesPerBar );
        }
    }

    if ( interpolatedValues.size() < 1 )
        return;
    
    switch ( m_mode )
    {
        case ( Bars ): paintBars( interpolatedValues ); break;
        case ( Wave ): paintWave( interpolatedValues ); break;
        case ( Waterfall ): paintWaterfall( interpolatedValues ); break;
        case ( Waves3D ): paint3DWaves( interpolatedValues ); break;
        case ( Channels3D ): /*paint3DChannels();*/ break;
        default: paintBars( interpolatedValues ); break;
    }
}

void AnalyzerGlWidget::setMode( AnalyzerMode mode )
{
    m_mode = mode;
}

void AnalyzerGlWidget::setFrequencyValues( QVector<int> frequency )
{
    m_frequencyValues = frequency;
}

