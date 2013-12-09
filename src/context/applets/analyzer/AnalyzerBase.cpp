/****************************************************************************************
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2009 Martin Sandsmark <martin.sandsmark@kde.org>                       *
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
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

#include <QTimer>


// INSTRUCTIONS
// 1. reimplement analyze()
// 2. if you want to manipulate the scope, reimplement transform()


Analyzer::Base::Base( QWidget *parent )
    : QGLWidget( parent )
    , m_fht( new FHT( log2( EngineController::DATAOUTPUT_DATA_SIZE ) ) )
    , m_renderTimer( new QTimer( this ) )
    , m_demoTimer( new QTimer( this ) )
{
    connect( EngineController::instance(), SIGNAL( playbackStateChanged() ), this, SLOT( playbackStateChanged() ) );

    setFps( 60 ); // Default unless changed by subclass
    m_demoTimer->setInterval( 33 );  // ~30 fps

    enableDemo( !EngineController::instance()->isPlaying() );

#ifdef Q_WS_X11
    connect( KWindowSystem::self(), SIGNAL( currentDesktopChanged( int ) ), this, SLOT( currentDesktopChanged() ) );
#endif

    connect( m_renderTimer, SIGNAL( timeout() ), this, SLOT( updateGL() ) );

    //initialize openGL context before managing GL calls
    makeCurrent();

    connectSignals();
}


Analyzer::Base::~Base()
{
    delete m_fht;
}

void
Analyzer::Base::connectSignals()
{
    DEBUG_BLOCK

    if( m_renderTimer->isActive() )
        return;

    connect( EngineController::instance(), SIGNAL( audioDataReady( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ),
        this, SLOT( processData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ) );
    connect( m_demoTimer, SIGNAL( timeout() ), this, SLOT( demo() ) );
    m_renderTimer->start();
}

void
Analyzer::Base::disconnectSignals()
{
    DEBUG_BLOCK

    disconnect( EngineController::instance(), SIGNAL( audioDataReady( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ),
        this, SLOT( processData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > & ) ) );
    m_demoTimer->disconnect( this );
    m_renderTimer->stop();
}

void
Analyzer::Base::currentDesktopChanged()
{
    // Optimization for X11/Linux desktops:
    // Don't update the analyzer if Amarok is not on the active virtual desktop.

    if( The::mainWindow()->isOnCurrentDesktop() )
        connectSignals();
    else
        disconnectSignals();
}

void
Analyzer::Base::playbackStateChanged()
{
    enableDemo( !EngineController::instance()->isPlaying() );
}

void
Analyzer::Base::enableDemo( bool enable )
{
    enable ? m_demoTimer->start() : m_demoTimer->stop();
}

void
Analyzer::Base::hideEvent( QHideEvent * )
{
    QTimer::singleShot( 0, this, SLOT( disconnectSignals() ) );
}

void
Analyzer::Base::showEvent( QShowEvent * )
{
    QTimer::singleShot( 0, this, SLOT( connectSignals() ) );
}

void
Analyzer::Base::transform( QVector<float> &scope ) //virtual
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

void
Analyzer::Base::processData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > &thescope )
{
    if( thescope.isEmpty() || thescope[Phonon::AudioDataOutput::LeftChannel].size() != m_fht->size() )
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

void
Analyzer::Base::demo() //virtual
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

void
Analyzer::Base::interpolate( const QVector<float> &inVec, QVector<float> &outVec ) const
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
Analyzer::Base::setFps( int fps )
{
    m_renderTimer->setInterval( 1000 / fps );
}


#include "AnalyzerBase.moc"
