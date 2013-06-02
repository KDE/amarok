/****************************************************************************************
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2009 Martin Sandsmark <martin.sandsmark@kde.org>                       *
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

#include "AnalyzerBase.h"

#include "core/support/Debug.h"
#include "EngineController.h"
#include "MainWindow.h"

#include <cmath> // interpolate()

#include <KWindowSystem>

#include <QEvent> // event()
#include <QPainter>


// INSTRUCTIONS
// 1. reimplement analyze(), and paint to canvas(), Base2D will update the widget when you return control to it
// 2. if you want to manipulate the scope, reimplement transform()


template<class W>
Analyzer::Base<W>::Base( QWidget *parent )
    : W( parent )
    , m_fht( new FHT( log2( EngineController::DATAOUTPUT_DATA_SIZE ) ) )
{}

template<class W> void
Analyzer::Base<W>::transform( QVector<float> &scope ) //virtual
{
    //this is a standard transformation that should give
    //an FFT scope that has bands for pretty analyzers

    float *front = static_cast<float*>( &scope.front() );

    float* f = new float[ m_fht->size() ];
    m_fht->copy( &f[0], front );
    m_fht->logSpectrum( front, &f[0] );
    m_fht->scale( front, 1.0 / 20 );

    scope.resize( m_fht->size() / 2 ); //second half of values are rubbish
    delete [] f;
}

template<class W> void
Analyzer::Base<W>::processData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > &thescope )
{
    if( thescope.isEmpty() )
        return;

    QVector<float> scope( m_fht->size() );

    for( uint x = 0; ( int )x < m_fht->size(); ++x )
    {
        if( thescope.size() == 1 )  // Mono
        {
            scope[x] = double( thescope[Phonon::AudioDataOutput::LeftChannel][x] );
        }
        else     // Anything > Mono is treated as Stereo
        {
            scope[x] = double( thescope[Phonon::AudioDataOutput::LeftChannel][x]
                               + thescope[Phonon::AudioDataOutput::RightChannel][x] )
                       / ( 2 * ( 1 << 15 ) ); // Average between the channels
        }
    }

    transform( scope );
    analyze( scope );
}

template<class W> void
Analyzer::Base<W>::paused() //virtual
{}

template<class W> void
Analyzer::Base<W>::demo() //virtual
{
    static int t = 201;

    if( t > 300 )
        t = 1; //0 = wasted calculations

    if( t < 201 )
    {
        QVector<float> s( 512 );

        const double dt = double( t ) / 200;
        for( int i = 0; i < s.size(); ++i )
            s[i] = dt * ( sin( M_PI + ( i * M_PI ) / s.size() ) + 1.0 );

        analyze( s );
    }
    else
        analyze( QVector<float>( 1, 0 ) );

    ++t;
}



Analyzer::Base2D::Base2D( QWidget *parent )
    : Base<QWidget>( parent )
{
    connect( EngineController::instance(), SIGNAL( playbackStateChanged() ), this, SLOT( playbackStateChanged() ) );

    m_demoTimer.setInterval( 33 );

    enableDemo( !EngineController::instance()->isPlaying() );

    m_renderTimer.setInterval( 20 ); //~50 FPS
    connect( &m_renderTimer, SIGNAL( timeout() ), this, SLOT( update() ) );

#ifdef Q_WS_X11
    connect( KWindowSystem::self(), SIGNAL( currentDesktopChanged( int ) ), this, SLOT( connectSignals() ) );
#endif

    connectSignals();
}

void Analyzer::Base2D::connectSignals()
{
    DEBUG_BLOCK

    // Optimization for X11/Linux desktops:
    // Don't update the analyzer if Amarok is not on the active virtual desktop.
    //
    // FIXME Get rid of the code duplication in both base classes

    static bool startup = true;

    if( ( The::mainWindow()->isOnCurrentDesktop() && The::mainWindow()->isVisible() ) || startup )
    {
        connect( EngineController::instance(), SIGNAL( audioDataReady( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ),
            this, SLOT( processData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ) );
        connect( &m_demoTimer, SIGNAL( timeout() ), this, SLOT( demo() ) );
        m_renderTimer.start();
        startup = false;
    }
    else
    {
        disconnect( EngineController::instance(), SIGNAL( audioDataReady( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ),
            this, SLOT( processData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ) );
        disconnect( &m_demoTimer, SIGNAL( timeout() ), this, SLOT( demo() ) );
        m_renderTimer.stop();
    }
}

void Analyzer::Base2D::resizeEvent( QResizeEvent *e )
{
    QWidget::resizeEvent( e );

    m_canvas = QPixmap( size() );
    m_canvas.fill( Qt::transparent );
}

void Analyzer::Base2D::paintEvent( QPaintEvent* )
{
    DEBUG_BLOCK

    if( m_canvas.isNull() )
        return;

    QPainter painter( this );
    painter.drawPixmap( rect(), m_canvas );
}

void Analyzer::Base2D::playbackStateChanged()
{
    enableDemo( !EngineController::instance()->isPlaying() );
}



Analyzer::Base3D::Base3D( QWidget *parent )
    : Base<QGLWidget>( parent )
{
    connect( EngineController::instance(), SIGNAL( playbackStateChanged() ), this, SLOT( playbackStateChanged() ) );

    m_demoTimer.setInterval( 33 );

    enableDemo( !EngineController::instance()->isPlaying() );

    m_renderTimer.setInterval( 17 ); //~60 FPS
    connect( &m_renderTimer, SIGNAL( timeout() ), this, SLOT( updateGL() ) );

#ifdef Q_WS_X11
    connect( KWindowSystem::self(), SIGNAL( currentDesktopChanged( int ) ), this, SLOT( connectSignals() ) );
#endif

    connectSignals();
}

void Analyzer::Base3D::playbackStateChanged()
{
    enableDemo( !EngineController::instance()->isPlaying() );
}

void Analyzer::Base3D::connectSignals()
{
    DEBUG_BLOCK

    // Optimization for X11/Linux desktops:
    // Don't update the analyzer if Amarok is not on the active virtual desktop.
    //
    // FIXME Get rid of the code duplication in both base classes

    static bool startup = true;

    if( ( The::mainWindow()->isOnCurrentDesktop() && The::mainWindow()->isVisible() ) || startup )
    {
        connect( EngineController::instance(), SIGNAL( audioDataReady( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ),
            this, SLOT( processData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ) );
        connect( &m_demoTimer, SIGNAL( timeout() ), this, SLOT( demo() ) );
        m_renderTimer.start();
        startup = false;
    }
    else
    {
        disconnect( EngineController::instance(), SIGNAL( audioDataReady( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ),
            this, SLOT( processData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ) );
        disconnect( &m_demoTimer, SIGNAL( timeout() ), this, SLOT( demo() ) );
        m_renderTimer.stop();
    }
}


void
Analyzer::interpolate( const QVector<float> &inVec, QVector<float> &outVec ) //static
{
    double pos = 0.0;
    const double step = ( double )inVec.size() / outVec.size();

    for( int i = 0; i < outVec.size(); ++i, pos += step )
    {
        const double error = pos - std::floor( pos );
        const unsigned long offset = ( unsigned long )pos;

        long indexLeft = offset + 0;

        if( indexLeft >= inVec.size() )
            indexLeft = inVec.size() - 1;

        long indexRight = offset + 1;

        if( indexRight >= inVec.size() )
            indexRight = inVec.size() - 1;

        outVec[i] = inVec[indexLeft ] * ( 1.0 - error ) +
                    inVec[indexRight] * error;
    }
}

void
Analyzer::initSin( QVector<float> &v, const uint size ) //static
{
    double step = ( M_PI * 2 ) / size;
    double radian = 0;

    for( uint i = 0; i < size; i++ )
    {
        v.push_back( sin( radian ) );
        radian += step;
    }
}
