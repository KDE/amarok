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

#include "BallsAnalyzer.h"

#include <QStandardPaths>

#include <QImage>

#include <cmath>
#include <cstdlib>
#include <sys/time.h>


inline float myfabsf( float f )
{
    return f < 0.f ? -f : f;
}


class Ball
{
public:
    Ball() : x( drand48() - drand48() ), y( 1 - 2.0 * drand48() ),
        z( drand48() ), vx( 0.0 ), vy( 0.0 ), vz( 0.0 ),
        mass( 0.01 + drand48() / 10.0 )
        //,color( (float[3]) { 0.0, drand48()*0.5, 0.7 + drand48() * 0.3 } )
    {
        //this is because GCC < 3.3 can't compile the above line, we aren't sure why though
        color[0] = 0.0; color[1] = drand48() * 0.5; color[2] = 0.7 + drand48() * 0.3;
    };

    float x, y, z, vx, vy, vz, mass;
    float color[3];

    void updatePhysics( float dT )
    {
        x += vx * dT;                // position
        y += vy * dT;                // position
        z += vz * dT;                // position
        if( y < -0.8 ) vy = myfabsf( vy );
        if( y > 0.8 ) vy = -myfabsf( vy );
        if( z < 0.1 ) vz = myfabsf( vz );
        if( z > 0.9 ) vz = -myfabsf( vz );
        vx += ( ( x > 0 ) ? 4.94 : -4.94 ) * dT;  // G-force
        vx *= ( 1 - 2.9 * dT );          // air friction
        vy *= ( 1 - 2.9 * dT );          // air friction
        vz *= ( 1 - 2.9 * dT );          // air friction
    }
};

class Paddle
{
public:
    Paddle( float xPos ) : onLeft( xPos < 0 ), mass( 1.0 ),
        X( xPos ), x( xPos ), vx( 0.0 ) {};

    void updatePhysics( float dT )
    {
        x += vx * dT;                // position
        vx += ( 1300 * ( X - x ) / mass ) * dT;    // elasticity
        vx *= ( 1 - 4.0 * dT );          // air friction
    }

    void renderGL()
    {
        glBegin( GL_TRIANGLE_STRIP );
        glColor3f( 0.0f, 0.1f, 0.3f );
        glVertex3f( x, -1.0f, 0.0 );
        glVertex3f( x, 1.0f, 0.0 );
        glColor3f( 0.1f, 0.2f, 0.6f );
        glVertex3f( x, -1.0f, 1.0 );
        glVertex3f( x, 1.0f, 1.0 );
        glEnd();
    }

    void bounce( Ball * ball )
    {
        if( onLeft && ball->x < x )
        {
            ball->vx = vx * mass / ( mass + ball->mass ) + myfabsf( ball->vx );
            ball->vy = ( drand48() - drand48() ) * 1.8;
            ball->vz = ( drand48() - drand48() ) * 0.9;
            ball->x = x;
        }
        else if( !onLeft && ball->x > x )
        {
            ball->vx = vx * mass / ( mass + ball->mass ) - myfabsf( ball->vx );
            ball->vy = ( drand48() - drand48() ) * 1.8;
            ball->vz = ( drand48() - drand48() ) * 0.9;
            ball->x = x;
        }
    }

    void impulse( float strength )
    {
        if( ( onLeft && strength > vx ) || ( !onLeft && strength < vx ) )
            vx += strength;
    }

private:
    bool onLeft;
    float mass, X, x, vx;
};


BallsAnalyzer::BallsAnalyzer( QWidget *parent ):
    Analyzer::Base( parent )
{
    setObjectName( "Balls" );

    m_ballTexture = bindTexture( QImage( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/ball.png" ) ) );
    m_gridTexture = bindTexture( QImage( QStandardPaths::locate( QStandardPaths::GenericDataLocation, "amarok/images/grid.png" ) ) );

    m_leftPaddle = new Paddle( -1.0 );
    m_rightPaddle = new Paddle( 1.0 );
    for( int i = 0; i < NUMBER_OF_BALLS; i++ )
        m_balls.append( new Ball() );

    m_show.colorK = 0.0;
    m_show.gridScrollK = 0.0;
    m_show.gridEnergyK = 0.0;
    m_show.camRot = 0.0;
    m_show.camRoll = 0.0;
    m_show.peakEnergy = 1.0;
    m_frame.silence = true;
    m_frame.energy = 0.0;
    m_frame.dEnergy = 0.0;
}

BallsAnalyzer::~BallsAnalyzer()
{
    deleteTexture( m_ballTexture );
    deleteTexture( m_gridTexture );
    delete m_leftPaddle;
    delete m_rightPaddle;

    qDeleteAll( m_balls );
}

void BallsAnalyzer::initializeGL()
{
    // Set a smooth shade model
    glShadeModel( GL_SMOOTH );

    // Disable depth test (all is drawn 'z-sorted')
    glDisable( GL_DEPTH_TEST );

    // Set blending function (Alpha addition)
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    // Clear frame with a black background
    glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );
}

void BallsAnalyzer::resizeGL( int w, int h )
{
    // Setup screen. We're going to manually do the perspective projection
    glViewport( 0, 0, ( GLint )w, ( GLint )h );
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glFrustum( -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 4.5f );

    // Get the aspect ratio of the screen to draw 'circular' particles
    float ratio = ( float )w / ( float )h;
    if( ratio >= 1.0 )
    {
        m_unitX = 0.34 / ratio;
        m_unitY = 0.34;
    }
    else
    {
        m_unitX = 0.34;
        m_unitY = 0.34 * ratio;
    }

    // Get current timestamp.
    timeval tv;
    gettimeofday( &tv, NULL );
    m_show.timeStamp = ( double )tv.tv_sec + ( double )tv.tv_usec / 1000000.0;
}

void BallsAnalyzer::analyze( const QVector<float> &s )
{
    // compute the dTime since the last call
    timeval tv;
    gettimeofday( &tv, NULL );
    double currentTime = ( double )tv.tv_sec + ( double )tv.tv_usec / 1000000.0;
    m_show.dT = currentTime - m_show.timeStamp;
    m_show.timeStamp = currentTime;

    // compute energy integrating frame's spectrum
    if( !s.empty() )
    {
        int bands = s.size();
        float currentEnergy = 0,
              maxValue = 0;
        // integrate spectrum -> energy
        for( int i = 0; i < bands; i++ )
        {
            float value = s[i];
            currentEnergy += value;
            if( value > maxValue )
                maxValue = value;
        }
        currentEnergy *= 100.0 / ( float )bands;
        // emulate a peak detector: currentEnergy -> peakEnergy (3tau = 30 seconds)
        m_show.peakEnergy = 1.0 + ( m_show.peakEnergy - 1.0 ) * exp( - m_show.dT / 10.0 );
        if( currentEnergy > m_show.peakEnergy )
            m_show.peakEnergy = currentEnergy;
        // check for silence
        m_frame.silence = currentEnergy < 0.001;
        // normalize frame energy against peak energy and compute frame stats
        currentEnergy /= m_show.peakEnergy;
        m_frame.dEnergy = currentEnergy - m_frame.energy;
        m_frame.energy = currentEnergy;
    }
    else
        m_frame.silence = true;

    // limit max dT to 0.05 and update color and scroll constants
    if( m_show.dT > 0.05 )
        m_show.dT = 0.05;
    m_show.colorK += m_show.dT * 0.4;
    if( m_show.colorK > 3.0 )
        m_show.colorK -= 3.0;
    m_show.gridScrollK += 0.2 * m_show.peakEnergy * m_show.dT;

    // Roll camera up/down handling the beat
    m_show.camRot += m_show.camRoll * m_show.dT;        // position
    m_show.camRoll -= 400 * m_show.camRot * m_show.dT;    // elasticity
    m_show.camRoll *= ( 1 - 2.0 * m_show.dT );      // friction
    if( !m_frame.silence && m_frame.dEnergy > 0.4 )
        m_show.camRoll += m_show.peakEnergy * 2.0;

    if( ( m_show.gridEnergyK > 0.05 ) || ( !m_frame.silence && m_frame.dEnergy < -0.3 ) )
    {
        m_show.gridEnergyK *= exp( -m_show.dT / 0.1 );
        if( -m_frame.dEnergy > m_show.gridEnergyK )
            m_show.gridEnergyK = -m_frame.dEnergy * 2.0;
    }

    for( Ball * ball : m_balls )
    {
        ball->updatePhysics( m_show.dT );
        if( ball->x < 0 )
            m_leftPaddle->bounce( ball );
        else
            m_rightPaddle->bounce( ball );
    }

    // Update physics of paddles
    m_leftPaddle->updatePhysics( m_show.dT );
    m_rightPaddle->updatePhysics( m_show.dT );
    if( !m_frame.silence )
    {
        m_leftPaddle->impulse( m_frame.energy * 3.0 + m_frame.dEnergy * 6.0 );
        m_rightPaddle->impulse( -m_frame.energy * 3.0 - m_frame.dEnergy * 6.0 );
    }
}

void BallsAnalyzer::paintGL()
{
    // Switch to MODEL matrix and clear screen
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glClear( GL_COLOR_BUFFER_BIT );

    // Draw scrolling grid
    float gridColor[4] = { 0.0, 1.0, 0.6, m_show.gridEnergyK };
    drawScrollGrid( m_show.gridScrollK, gridColor );

    glRotatef( m_show.camRoll / 2.0, 1, 0, 0 );

    // Translate the drawing plane
    glTranslatef( 0.0f, 0.0f, -1.8f );

    // Draw upper/lower planes and paddles
    drawHFace( -1.0 );
    drawHFace( 1.0 );
    m_leftPaddle->renderGL();
    m_rightPaddle->renderGL();

    // Draw Balls
    if( m_ballTexture )
    {
        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, m_ballTexture );
    }
    else
        glDisable( GL_TEXTURE_2D );

    glEnable( GL_BLEND );

    for( Ball * ball : m_balls )
    {
        float color[3],
              angle = m_show.colorK;
        // Rotate the color based on 'angle' value [0,3)
        if( angle < 1.0 )
        {
            color[ 0 ] = ball->color[ 0 ] * ( 1 - angle ) + ball->color[ 1 ] * angle;
            color[ 1 ] = ball->color[ 1 ] * ( 1 - angle ) + ball->color[ 2 ] * angle;
            color[ 2 ] = ball->color[ 2 ] * ( 1 - angle ) + ball->color[ 0 ] * angle;
        }
        else if( angle < 2.0 )
        {
            angle -= 1.0;
            color[ 0 ] = ball->color[ 1 ] * ( 1 - angle ) + ball->color[ 2 ] * angle;
            color[ 1 ] = ball->color[ 2 ] * ( 1 - angle ) + ball->color[ 0 ] * angle;
            color[ 2 ] = ball->color[ 0 ] * ( 1 - angle ) + ball->color[ 1 ] * angle;
        }
        else
        {
            angle -= 2.0;
            color[ 0 ] = ball->color[ 2 ] * ( 1 - angle ) + ball->color[ 0 ] * angle;
            color[ 1 ] = ball->color[ 0 ] * ( 1 - angle ) + ball->color[ 1 ] * angle;
            color[ 2 ] = ball->color[ 1 ] * ( 1 - angle ) + ball->color[ 2 ] * angle;
        }
        // Draw the dot and update its physics also checking at bounces
        glColor3fv( color );
        drawDot3s( ball->x, ball->y, ball->z, 1.0 );
    }
    glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );
}

void BallsAnalyzer::drawDot3s( float x, float y, float z, float size )
{
    // Circular XY dot drawing functions
    float sizeX = size * m_unitX,
          sizeY = size * m_unitY,
          pXm = x - sizeX,
          pXM = x + sizeX,
          pYm = y - sizeY,
          pYM = y + sizeY;
    // Draw the Dot
    glBegin( GL_QUADS );
    glTexCoord2f( 0, 0 );    // Bottom Left
    glVertex3f( pXm, pYm, z );
    glTexCoord2f( 0, 1 );    // Top Left
    glVertex3f( pXm, pYM, z );
    glTexCoord2f( 1, 1 );    // Top Right
    glVertex3f( pXM, pYM, z );
    glTexCoord2f( 1, 0 );    // Bottom Right
    glVertex3f( pXM, pYm, z );
    glEnd();

    // Shadow XZ drawing functions
    float sizeZ = size / 10.0,
          pZm = z - sizeZ,
          pZM = z + sizeZ,
          currentColor[4];
    glGetFloatv( GL_CURRENT_COLOR, currentColor );
    float alpha = currentColor[3],
          topSide = ( y + 1 ) / 4,
          bottomSide = ( 1 - y ) / 4;
    // Draw the top shadow
    currentColor[3] = topSide * topSide * alpha;
    glColor4fv( currentColor );
    glBegin( GL_QUADS );
    glTexCoord2f( 0, 0 );    // Bottom Left
    glVertex3f( pXm, 1, pZm );
    glTexCoord2f( 0, 1 );    // Top Left
    glVertex3f( pXm, 1, pZM );
    glTexCoord2f( 1, 1 );    // Top Right
    glVertex3f( pXM, 1, pZM );
    glTexCoord2f( 1, 0 );    // Bottom Right
    glVertex3f( pXM, 1, pZm );
    glEnd();
    // Draw the bottom shadow
    currentColor[3] = bottomSide * bottomSide * alpha;
    glColor4fv( currentColor );
    glBegin( GL_QUADS );
    glTexCoord2f( 0, 0 );    // Bottom Left
    glVertex3f( pXm, -1, pZm );
    glTexCoord2f( 0, 1 );    // Top Left
    glVertex3f( pXm, -1, pZM );
    glTexCoord2f( 1, 1 );    // Top Right
    glVertex3f( pXM, -1, pZM );
    glTexCoord2f( 1, 0 );    // Bottom Right
    glVertex3f( pXM, -1, pZm );
    glEnd();
}

void BallsAnalyzer::drawHFace( float y )
{
    glBegin( GL_TRIANGLE_STRIP );
    glColor3f( 0.0f, 0.1f, 0.2f );
    glVertex3f( -1.0f, y, 0.0 );
    glVertex3f( 1.0f, y, 0.0 );
    glColor3f( 0.1f, 0.6f, 0.5f );
    glVertex3f( -1.0f, y, 2.0 );
    glVertex3f( 1.0f, y, 2.0 );
    glEnd();
}

void BallsAnalyzer::drawScrollGrid( float scroll, float color[4] )
{
    if( !m_gridTexture )
        return;
    glMatrixMode( GL_TEXTURE );
    glLoadIdentity();
    glTranslatef( 0.0, -scroll, 0.0 );
    glMatrixMode( GL_MODELVIEW );
    float backColor[4] = { 1.0, 1.0, 1.0, 0.0 };
    for( int i = 0; i < 3; i++ )
        backColor[ i ] = color[ i ];
    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, m_gridTexture );
    glEnable( GL_BLEND );
    glBegin( GL_TRIANGLE_STRIP );
    glColor4fv( color );    // top face
    glTexCoord2f( 0.0f, 1.0f );
    glVertex3f( -1.0f, 1.0f, -1.0f );
    glTexCoord2f( 1.0f, 1.0f );
    glVertex3f( 1.0f, 1.0f, -1.0f );
    glColor4fv( backColor );    // central points
    glTexCoord2f( 0.0f, 0.0f );
    glVertex3f( -1.0f, 0.0f, -3.0f );
    glTexCoord2f( 1.0f, 0.0f );
    glVertex3f( 1.0f, 0.0f, -3.0f );
    glColor4fv( color );    // bottom face
    glTexCoord2f( 0.0f, 1.0f );
    glVertex3f( -1.0f, -1.0f, -1.0f );
    glTexCoord2f( 1.0f, 1.0f );
    glVertex3f( 1.0f, -1.0f, -1.0f );
    glEnd();
    glDisable( GL_BLEND );
    glDisable( GL_TEXTURE_2D );
    glMatrixMode( GL_TEXTURE );
    glLoadIdentity();
    glMatrixMode( GL_MODELVIEW );
}
