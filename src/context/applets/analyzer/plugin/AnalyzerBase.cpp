/****************************************************************************************
 * Copyright (c) 2003 Max Howell <max.howell@methylblue.com>                            *
 * Copyright (c) 2009 Martin Sandsmark <martin.sandsmark@kde.org>                       *
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2017 Malte Veerman <malte.veerman@gmail.com>                           *
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
#include "AnalyzerWorker.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "MainWindow.h"

#include <cmath>

#ifdef Q_WS_X11
#include <KWindowSystem>
#endif

#include <QQuickWindow>
#include <QTimer>


// INSTRUCTIONS
// 1. Reimplement QQuickFramebufferObject::createRenderer().
// 2. Reimplement Analyzer::Base::createWorker().
// 3. Set your preferred scope width with setScopeSize().


Analyzer::Base::Base( QQuickItem *parent )
    : QQuickFramebufferObject( parent )
    , m_sampleRate( 44100 )
    , m_scopeSize( 0 )
    , m_worker( Q_NULLPTR )
{
    DEBUG_BLOCK

    qRegisterMetaType<WindowFunction>();

    m_minFreq = config().readEntry( "minFreq", 50.0 );
    m_maxFreq = config().readEntry( "maxFreq", 15000.0 );

    connect( The::engineController(), &EngineController::trackChanged, this, &Base::refreshSampleRate );
    connect( The::engineController(), &EngineController::trackMetadataChanged, this, &Base::refreshSampleRate );

#ifdef Q_WS_X11
    connect( KWindowSystem::self(), &KWindowSystem::currentDesktopChanged, this, &Base::currentDesktopChanged );
#endif

    QTimer::singleShot( 0, this, &Base::connectSignals );
}

Analyzer::Base::~Base()
{
    DEBUG_BLOCK

    m_worker->deleteLater();
    m_worker = Q_NULLPTR;
    m_workerThread.quit();
    m_workerThread.wait();
}

void
Analyzer::Base::connectSignals()
{
    DEBUG_BLOCK

    if( !m_worker )
    {
        m_worker = createWorker();
        m_worker->setSampleSize( sampleSize() );
        m_worker->setScopeSize( m_scopeSize );
        m_worker->setWindowFunction( windowFunction() );
        m_worker->moveToThread( &m_workerThread );
        m_workerThread.start();

        connect( this, &Base::calculateExpFactorNeeded, m_worker, &Worker::calculateExpFactor );
        connect( this, &Base::windowFunctionChanged, m_worker, &Worker::setWindowFunction );
        connect( this, &Base::sampleSizeChanged, m_worker, &Worker::setSampleSize );
        connect( this, &Base::scopeSizeChanged, m_worker, &Worker::setScopeSize );
        connect( The::engineController(), &EngineController::playbackStateChanged, m_worker, &Worker::playbackStateChanged );
        connect( The::engineController(), &EngineController::audioDataReady, m_worker, &Worker::receiveData, Qt::DirectConnection );

        setSampleSize( config().readEntry( "sampleSize", 4096 ) );
        setWindowFunction( (WindowFunction) config().readEntry( "windowFunction", (int)Hann ) );
        emit calculateExpFactorNeeded( m_minFreq, m_maxFreq, m_sampleRate);
    }
}

void
Analyzer::Base::disconnectSignals()
{
    DEBUG_BLOCK

    if( m_worker )
        disconnect( The::engineController(), &EngineController::audioDataReady, m_worker, &Worker::receiveData );
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
Analyzer::Base::refreshSampleRate()
{
    const auto currentTrack = The::engineController()->currentTrack();
    int sampleRate = currentTrack ? currentTrack->sampleRate() : 44100;

    if( m_sampleRate == sampleRate )
        return;

    m_sampleRate = sampleRate;

    emit calculateExpFactorNeeded( m_minFreq, m_maxFreq, m_sampleRate );
}

KConfigGroup
Analyzer::Base::config() const
{
    return Amarok::config( QStringLiteral( "Context" ) ).group( "Analyzer" );
}

void
Analyzer::Base::setScopeSize( int scopeSize )
{
    if( scopeSize <= 0 )
    {
        debug() << "Scope size must be greater than zero";
        return;
    }

    if( m_scopeSize == scopeSize )
        return;

    m_scopeSize = scopeSize;
    emit scopeSizeChanged( scopeSize );
    emit calculateExpFactorNeeded( m_minFreq, m_maxFreq, m_sampleRate );
}

void
Analyzer::Base::setMaxFreq( qreal maxFreq )
{
    DEBUG_BLOCK

    debug() << "Set maximum frequency to:" << maxFreq;

    if( m_maxFreq == maxFreq || maxFreq < 0.0 )
        return;

    config().writeEntry( "maxFreq", maxFreq );
    m_maxFreq = maxFreq;
    emit maxFreqChanged();
    emit calculateExpFactorNeeded( m_minFreq, m_maxFreq, m_sampleRate );
}

void
Analyzer::Base::setMinFreq( qreal minFreq )
{
    DEBUG_BLOCK

    debug() << "Set minimum frequency to:" << minFreq;

    if( m_minFreq == minFreq || minFreq <= 0.0 )
        return;

    config().writeEntry( "minFreq", minFreq );
    m_minFreq = minFreq;
    emit minFreqChanged();
    emit calculateExpFactorNeeded( m_minFreq, m_maxFreq, m_sampleRate );
}

Analyzer::Base::WindowFunction
Analyzer::Base::windowFunction() const
{
    return (WindowFunction)config().readEntry( "windowFunction", (int)Hann );
}

void
Analyzer::Base::setWindowFunction( WindowFunction windowFunction )
{
    DEBUG_BLOCK

    debug() << "Set window function to:" << windowFunction;

    config().writeEntry( "windowFunction", (int)windowFunction );
    emit windowFunctionChanged( windowFunction );
}

int Analyzer::Base::sampleSize() const
{
    return config().readEntry( "sampleSize", 2048 );
}

void
Analyzer::Base::setSampleSize( uint sampleSize )
{
    DEBUG_BLOCK

    debug() << "Set sample size to:" << sampleSize;

    if( sampleSize < (int) EngineController::DATAOUTPUT_DATA_SIZE )
    {
        warning() << "Sample size must be at least" << EngineController::DATAOUTPUT_DATA_SIZE;
        sampleSize = EngineController::DATAOUTPUT_DATA_SIZE;
    }

    config().writeEntry( "sampleSize", sampleSize );
    emit sampleSizeChanged( sampleSize );
    emit calculateExpFactorNeeded( m_minFreq, m_maxFreq, m_sampleRate );
}

const Analyzer::Worker *
Analyzer::Base::worker() const
{
    return const_cast<const Worker*>( m_worker );
}

