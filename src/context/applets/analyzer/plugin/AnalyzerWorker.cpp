/*
 * Copyright 2018  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AnalyzerWorker.h"

#include "core/support/Debug.h"
#include "EngineController.h"

#include <QThread>
#include <QTimer>


Analyzer::Worker::Worker()
    : m_currentScope( QVector<double>( 1, 0.0 ) )
    , m_size( 0 )
    , m_windowFunction( Base::Hann )
    , m_expectedDataTime( 20 )
    , m_demoT( 201 )
    , m_lastUpdate()
    , m_demoTimer( new QTimer( this ) )
    , m_processTimer( new QTimer( this ) )
{
    m_lastUpdate.start();
    m_in = (double*) fftw_malloc( m_size * sizeof( double ) );
    m_out = (std::complex<double>*) fftw_malloc( ( m_size / 2 + 1 ) * sizeof( std::complex<double> ) );
    m_plan = fftw_plan_dft_r2c_1d( m_size, m_in, reinterpret_cast<fftw_complex*>( m_out ), FFTW_ESTIMATE );

    m_demoTimer->setInterval( Analyzer::Base::DEMO_INTERVAL );
    m_processTimer->setInterval( PROCESSING_INTERVAL );
    if( EngineController::instance()->isPlaying() )
        m_processTimer->start();
    else
        m_demoTimer->start();

    connect( m_demoTimer, &QTimer::timeout, this, &Worker::demo );
    connect( m_processTimer, &QTimer::timeout, this, &Worker::processData );
}

Analyzer::Worker::~Worker()
{
    fftw_destroy_plan( m_plan );
    fftw_free( m_in );
    fftw_free( m_out );
}

void Analyzer::Worker::receiveData( const QMap<Phonon::AudioDataOutput::Channel, QVector<qint16> > &newData )
{
    const int newDataSize = EngineController::DATAOUTPUT_DATA_SIZE;

    if( newData.isEmpty() || newData[Phonon::AudioDataOutput::LeftChannel].size() != newDataSize )
        return;

    m_rawInMutex.lock();

    for( int x = 0; x < newDataSize; x++ )
    {
        if( newData.size() == 1 )  // Mono
        {
            m_rawIn << double( newData[Phonon::AudioDataOutput::LeftChannel][x] );
        }
        else     // Anything > Mono is treated as Stereo
        {
            m_rawIn << ( double( newData[Phonon::AudioDataOutput::LeftChannel][x] )
            + double( newData[Phonon::AudioDataOutput::RightChannel][x] ) )
            / 2; // Average between the channels
        }
        m_rawIn.last() /= ( 1 << 15 ); // Scale to [0, 1]
    }

    m_rawInMutex.unlock();
}

void Analyzer::Worker::processData()
{
    int timeElapsed = m_lastUpdate.elapsed();

    // Delay if processing is too fast
    if( timeElapsed < m_expectedDataTime - 1 )
        QThread::currentThread()->msleep( m_expectedDataTime - timeElapsed - 1 );

    applyWindowFunction();
}

void Analyzer::Worker::applyWindowFunction()
{
    m_rawInMutex.lock();

    if( m_rawIn.size() < (int)m_size )
    {
        m_rawInMutex.unlock();
        return;
    }

    const int newDataSize = EngineController::DATAOUTPUT_DATA_SIZE;

    while( m_rawIn.size() > (int)m_size + DATA_BUFFER_SIZE * newDataSize )
        m_rawIn.removeFirst();

    // Apply window function
    for( uint i = 0; i < m_size; i++ )
    {
        double windowFactor;
        switch( m_windowFunction )
        {
            case Base::Rectangular:
            {
                windowFactor = 1.0;
                break;
            }
            case Base::Hann:
            {
                windowFactor = ( 1.0 - cos( 2.0 * M_PI * i / ( m_size - 1 ) ) ) / 2.0;
                break;
            }
            case Base::Nuttall:
            {
                const double a = 0.355768;
                const double b = 0.487396 * cos( 2 * M_PI * i / ( m_size - 1 ) );
                const double c = 0.144232 * cos( 4 * M_PI * i / ( m_size - 1 ) );
                const double d = 0.012604 * cos( 6 * M_PI * i / ( m_size - 1 ) );
                windowFactor = a - b + c - d;
                break;
            }
            case Base::Lanczos:
            {
                const double x = 2.0 * i / ( m_size - 1 ) - 1;
                windowFactor = sin( M_PI * x ) / M_PI / x;
                break;
            }
            case Base::Sine:
            {
                windowFactor = ( M_PI * i ) / ( m_size - 1 );
                break;
            }
        };

        if( i < newDataSize )
            m_in[i] = m_rawIn.takeFirst() * windowFactor;
        else
            m_in[i] = m_rawIn.at( i - newDataSize ) * windowFactor;
    }

    m_rawInMutex.unlock();

    fftw_execute( m_plan );
    makeScope();
}

void Analyzer::Worker::makeScope()
{
    for( const auto& band : m_notInterpolatedScopeBands )
    {
        m_currentScope[band.scopeIndex] = 0.0;
        uint numValues = 0;
        for( long k = std::lround( std::ceil( band.lowerK ) ); k <= std::lround( std::floor( band.upperK ) ); k++ )
        {
            m_currentScope[band.scopeIndex] += std::abs( m_out[k] ) * sqrt( k );
            numValues++;
        }
        m_currentScope[band.scopeIndex] /= numValues;
        m_currentScope[band.scopeIndex] /= m_size / 2;
    }

    // monotone cubic interpolation
    if( !m_interpolatedScopeBands.isEmpty() )
    {
        QVector<QPointF> data;
        for( uint k = 0; k < m_size / 2 + 1 && k <= m_interpolatedScopeBands.last().midK; k++ )
        {
            data << QPointF( k, std::abs( m_out[k] ) * sqrt( k ) / m_size * 2 );
        }
        // Get consecutive differences and slopes
        QVector<double> dys, dxs, ms;
        for( int i = 0; i < data.size() - 1; i++ )
        {
            double dx = data[i + 1].x() - data[i].x();
            double dy = data[i + 1].y() - data[i].y();
            dxs << dx;
            dys << dy;
            ms << dy / dx;
        }
        // Get degree-1 coefficients
        QVector<double> c1s = QVector<double>() << ms[0];
        for( int i = 0; i < dxs.size() - 1; i++)
        {
            double m = ms[i], mNext = ms[i + 1];
            if( m * mNext <= 0 )
                c1s << 0.0;
            else
            {
                double dx_ = dxs[i], dxNext = dxs[i + 1], common = dx_ + dxNext;
                c1s << ( 3 * common / ( ( common + dxNext ) / m + ( common + dx_ ) / mNext ) );
            }
        }
        c1s << ms.last();
        // Get degree-2 and degree-3 coefficients
        QVector<double> c2s, c3s;
        for( int i = 0; i < c1s.size() - 1; i++ )
        {
            double c1 = c1s[i], m_ = ms[i], invDx = 1 / dxs[i], common_ = c1 + c1s[i + 1] - m_ - m_;
            c2s << ( m_ - c1 - common_ ) * invDx;
            c3s << common_ * invDx * invDx;
        }
        // write interpolated data to scope
        for( auto &band : m_interpolatedScopeBands )
        {
            const double x = band.midK;
            auto &scope = m_currentScope[band.scopeIndex];

            // Search for the interval x is in, returning the corresponding y if x is one of the original xs
            int low = 0, mid, high = c3s.size() - 1;
            while ( low <= high )
            {
                mid = std::floor( 0.5 * ( low + high ) );
                double xHere = data[mid].x();
                if( xHere < x )
                    low = mid + 1;
                else if( xHere > x )
                    high = mid - 1;
                else
                    scope = data[mid].y();
            }
            int i = qMax( 0, high );

            // Interpolate
            double diff = x - data[i].x(), diffSq = diff * diff;
            scope = qMax( 0.0, data[i].y() + c1s[i] * diff + c2s[i] * diffSq + c3s[i] * diff * diffSq );
        }
    }

    analyze();
}

void Analyzer::Worker::setSampleSize( uint size )
{
    if( m_size == size )
        return;

    m_size = size;

    fftw_destroy_plan( m_plan );
    fftw_free( m_in );
    fftw_free( m_out );

    m_in = (double*) fftw_malloc( m_size * sizeof( double ) );
    m_out = (std::complex<double>*) fftw_malloc( ( m_size / 2 + 1 ) * sizeof( std::complex<double> ) );
    m_plan = fftw_plan_dft_r2c_1d( m_size, m_in, reinterpret_cast<fftw_complex*>( m_out ), FFTW_ESTIMATE );
}

void Analyzer::Worker::setWindowFunction( Base::WindowFunction windowFunction )
{
    if( m_windowFunction == windowFunction )
        return;

    m_windowFunction = windowFunction;
}

void Analyzer::Worker::setScopeSize( int size )
{
    m_currentScope.resize( size );
}

void Analyzer::Worker::calculateExpFactor( qreal minFreq, qreal maxFreq, int sampleRate )
{
    DEBUG_BLOCK

    if( minFreq <= 0.0 )
    {
        warning() << "Minimum frequency must be greater than zero!";
        minFreq = 1.0;
    }

    if( minFreq >= maxFreq )
    {
        warning() << "Minimum frequency must be smaller than maximum frequency!";
        maxFreq = minFreq + 1.0;
    }

    if( sampleRate == 0 )
    {
        debug() << "Reported impossible sample rate of zero. Assuming 44.1KHz.";
        sampleRate = 44100;
    }

    m_expFactor = pow( maxFreq / minFreq, 1.0 / m_currentScope.size() );
    m_expectedDataTime = std::floor( (qreal)EngineController::DATAOUTPUT_DATA_SIZE * 1000.0 / sampleRate );

    m_interpolatedScopeBands.clear();
    m_notInterpolatedScopeBands.clear();
    const uint outputSize = m_size / 2 + 1;

    for( int scopeIndex = 0; scopeIndex < m_currentScope.size(); scopeIndex++ )
    {
        BandInfo newBandInfo;
        newBandInfo.lowerFreq = minFreq * pow( m_expFactor, double( scopeIndex ) - 0.5 );
        newBandInfo.midFreq = minFreq * pow( m_expFactor, scopeIndex );
        newBandInfo.upperFreq = minFreq * pow( m_expFactor, double( scopeIndex ) + 0.5 );
        newBandInfo.lowerK = newBandInfo.lowerFreq / ( sampleRate / 2 ) * outputSize;
        newBandInfo.midK = newBandInfo.midFreq / ( sampleRate / 2 ) * outputSize;
        newBandInfo.upperK = newBandInfo.upperFreq / ( sampleRate / 2 ) * outputSize;
        newBandInfo.scopeIndex = scopeIndex;

        if ( std::floor( newBandInfo.upperK ) >= std::ceil( newBandInfo.lowerK ) )
            m_notInterpolatedScopeBands << newBandInfo;
        else
            m_interpolatedScopeBands << newBandInfo;
    }
}

void Analyzer::Worker::demo()
{
    if( m_demoT > 300 )
        m_demoT = 1; //0 = wasted calculations

    if( m_demoT < 201 )
    {
        const double dt = double( m_demoT ) / 200;
        for( int i = 0; i < m_currentScope.size(); ++i )
        {
            m_currentScope[i] = dt * ( sin( M_PI + ( i * M_PI ) / m_currentScope.size() ) + 1.0 );
        }
    }
    else
    {
        for( int i = 0; i < m_currentScope.size(); ++i )
        {
            m_currentScope[i] = 0.0;
        }
    }

    ++m_demoT;

    int timeElapsed = m_lastUpdate.elapsed();

    // Delay if interval is too low
    if( timeElapsed < Analyzer::Base::DEMO_INTERVAL - 1 )
        QThread::currentThread()->msleep( Analyzer::Base::DEMO_INTERVAL - 1 - timeElapsed );

    m_lastUpdate.restart();

    analyze();
}

void Analyzer::Worker::playbackStateChanged()
{
    bool playing = EngineController::instance()->isPlaying();
    playing ? m_demoTimer->stop() : m_demoTimer->start();
    playing ? m_processTimer->start() : m_processTimer->stop();
    resetDemo();
}
