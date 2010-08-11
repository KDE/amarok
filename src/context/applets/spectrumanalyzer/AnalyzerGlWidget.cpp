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

AnalyzerGlWidget::AnalyzerGlWidget( QGLFormat format, QColor fillColor )
    :QGLWidget( format )
    ,m_fillColor( fillColor )
    ,m_mode( Bars )
    ,m_showPeaks( false )
    ,m_showWave( false )
    ,m_peakSinkRate( 1.0f )    
    ,m_barsPerFrequency( 1.0f )
{
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
    glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );
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
    glEnable( GL_BLEND );
    
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

    glDisable( GL_BLEND );
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

QVector<GLubyte> AnalyzerGlWidget::getValueColor( int value )
{
    if( value < 0 )
        value = 0;
    
    QVector<GLubyte> result( 3 );

    if( value < 51 )
    {
        result[0] = m_fillColor.redF() * 1.2 * 255;
        result[1] = m_fillColor.greenF() * 1.077 * 255;
        result[2] = m_fillColor.blueF() * 255;
    }
    else if( value < 51 )
    {
        result[0] = 0;
        result[1] = 0;
        result[2] = ( value / 51 ) * 255;
    }
    else if( ( value > 51 ) && ( value < 102 ) )
    {
        result[0] = 0;
        result[1] = ( value / 102 ) * 255;
        result[2] = 255;
    }
    else if( ( value > 102 ) && ( value < 153 ) )
    {
        result[0] = 0;
        result[1] = 255;
        result[2] = (153 - value) * 5;
    }
    else if( ( value > 153 ) && ( value < 204 ) )
    {
        result[0] = ( value / 204 ) * 255;
        result[1] = 255;
        result[2] = 0;
    }
    else if( ( value > 204 ) && ( value < 255 ) )
    {
        result[0] = (255 - value) * 5;
        result[1] = 0;
        result[2] = 0;
    }
    else
    {
        result[0] = 255;
        result[1] = 0;
        result[2] = 0;
    }

    return result;
}

void AnalyzerGlWidget::paintWaterfall( QVector<int> values )
{
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    
    if( ( m_lastValues.size() > 0 ) && ( m_lastValues[0].size() != values.size() ) )
        m_lastValues.clear();

    while( m_lastValues.size() < 500 )
    {
        m_lastValues.append(QVector<int>(values.size()).fill(0));
    }
    
    GLubyte *bitmap = new GLubyte[m_lastValues.size()*values.size()*3]; //Space for a 512x(number of frequencys) Image with RGB Colors

    if( m_lastValues.size() > 512 )
        m_lastValues.removeAt( 0 );

    m_lastValues.append( values );

    for( int y = 0; y < m_lastValues.size() - 1; y++ )
    {
        for( int x = 0; x < values.size() * 3; x += 3 )
        {
            QVector<GLubyte> color2 = getValueColor( m_lastValues[y][x/3] );
            bitmap[x+(values.size() * y * 3)] = color2[0];
            bitmap[x+1+(values.size() * y * 3)] = color2[1];
            bitmap[x+2+(values.size() * y * 3)] = color2[2];
        }
    }

    glEnable( GL_TEXTURE_2D );
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, values.size(), m_lastValues.size(), 0, GL_RGB, GL_UNSIGNED_BYTE, bitmap );

    glBegin( GL_TRIANGLE_STRIP );
        glTexCoord2f( 1.0f, 0.0f ); glVertex3d( 1.0f, -1.0, -0.1 );
        glTexCoord2f( 1.0f, 1.0f ); glVertex3d( 1.0f,  1.0, -0.1 );
        glTexCoord2f( 0.0f, 0.0f ); glVertex3d( -1.0, -1.0, -0.1 );
        glTexCoord2f( 0.0f, 1.0f ); glVertex3d( -1.0,  1.0, -0.1 );
    glEnd();

    glDisable( GL_TEXTURE_2D );
}

void AnalyzerGlWidget::paint3DWaves( QVector<int> values )
{
    if( ( m_lastValues.size() > 0 ) && ( values.size() != m_lastValues[0].size() ) )
        m_lastValues.clear();
    
    while( m_lastValues.size() < 250 )
    {
        m_lastValues.append(QVector<int>(values.size()).fill(0));
    }
    
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
                glVertex3f( ( 1.5f / frequencys ) * x - 0.75f, 0.0078f * m_lastValues[y][x] - 0.5f, ( 2.0f / m_lastValues.size() ) * y - 1.0f );
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

    if( m_frequencyValues.size() < 1 )
        return;

    QVector<int> interpolatedValues;

    if( m_mode != Channels3D )
    {
        int numBars = 1;
        if( ( m_mode == Waterfall ) && ( m_barsPerFrequency < 0.25f ) )
        {
            numBars = m_frequencyValues.size() * 0.25f;
        }
        else
        {
            numBars = m_frequencyValues.size() * m_barsPerFrequency;
        }

        if ( numBars <= 0 )
            numBars = 1;

        if ( numBars > m_frequencyValues.size() )
        {
            numBars = m_frequencyValues.size();
        }

        if ( numBars != m_frequencyValues.size() )
        {
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
        else
        {
            interpolatedValues = m_frequencyValues;
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
        case ( Channels3D ): /*paint3DChannels();*/ break; //TODO: This needs to be correctly implemented first (see paint3DChannels(QMap<int,QVector<int>>)
        default: paintBars( interpolatedValues ); break;
    }
}

void AnalyzerGlWidget::setMode( int mode )
{
    switch ( mode )
    {
        case ( 0 ): m_mode = Bars; break;
        case ( 1 ): m_mode = Wave; break;
        case ( 2 ): m_mode = Waterfall; break;
        case ( 3 ): m_mode = Waves3D; break;
        case ( 4 ): m_mode = Channels3D; break;
        default: m_mode = Bars; break;
    }
}

int AnalyzerGlWidget::getMode()
{
    switch ( m_mode )
    {
        case ( Bars ): return 0;
        case ( Wave ): return 1;
        case ( Waterfall ): return 2;
        case ( Waves3D ): return 3;
        case ( Channels3D ): return 4;
        default: return 0;
    }
}

void AnalyzerGlWidget::setFrequencyValues( QVector<int> frequency )
{
    m_frequencyValues = frequency;
}

void AnalyzerGlWidget::keyPressEvent( QKeyEvent* event )
{
    emit keyPressed( event->key() );
    QWidget::keyPressEvent( event );
}


