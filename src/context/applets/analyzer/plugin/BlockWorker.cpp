/*
 * Copyright 2017  Malte Veerman <malte.veerman@gmail.com>
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
 *
 */

#include "BlockWorker.h"
#include "BlockAnalyzer.h"

#include "core/support/Debug.h"


BlockWorker::BlockWorker( int rows, int columns, qreal step, bool showFadebars )
    : m_step( step )
    , m_rows( rows )
    , m_columns( columns )
    , m_refreshTime( 16 )
    , m_showFadebars( showFadebars )
{
    m_yscale.resize( m_rows );
    const double PRO = 1; //PRO allows us to restrict the range somewhat

    for( int z = 0; z < m_rows; ++z )
        m_yscale[z] = 1 - log10( m_rows - z ) / log10( m_rows + PRO );

    m_store.resize( columns );
    m_fadebars.resize( columns );

    m_lastUpdate.start();
}

void BlockWorker::setRows( int rows )
{
    if( m_rows == rows )
        return;

    m_mutex.lock();
    m_rows = rows;
    m_yscale.resize( m_rows + 1 );

    const double PRO = 1; //PRO allows us to restrict the range somewhat

    for( int z = 0; z < m_rows; ++z )
        m_yscale[z] = 1 - log10( m_rows - z ) / log10( m_rows + PRO );

    m_mutex.unlock();
}

void BlockWorker::setColumns( int columns )
{
    if( m_columns == columns )
        return;

    m_columns = columns;
}

void BlockWorker::analyze()
{
    int timeElapsed = m_lastUpdate.elapsed();

    // only analyze if screen is fast enough
    if( timeElapsed < m_refreshTime - 1 )
        QThread::currentThread()->msleep( m_refreshTime - timeElapsed - 1 );

    const auto scopeData = scope();
    const int scopeSize = scopeData.size();

    timeElapsed = m_lastUpdate.restart();

    const qreal step = m_step * timeElapsed / 1000.0;
    const qreal fadeStep = (qreal)timeElapsed / 20.0;

    // lock m_store and m_fadebars
    QMutexLocker locker( &m_mutex );

    m_store.resize( scopeSize );
    m_fadebars.resize( scopeSize );

    for( int x = 0; x < scopeSize; ++x )
    {
        // determine y
        int y = 0;
        while( y < m_yscale.size() && scopeData.at(x) > m_yscale.at(y) )
            y++;

        auto &fadebars = m_fadebars[x];
        auto &store = m_store[x];

        // remove obscured fadebars
        while( !fadebars.isEmpty() && fadebars.last().y <= y )
            fadebars.removeLast();

        // remove completely faded fadebars
        while( !fadebars.isEmpty() && fadebars.first().intensity <= fadeStep )
            fadebars.removeFirst();

        // fade the rest
        for( auto &fadebar : fadebars )
            fadebar.intensity -= fadeStep;

        if( ( double )y < store )
        {
            // add new fadebar at old column height
            if( m_showFadebars )
                fadebars << Fadebar( store, BlockAnalyzer::FADE_SIZE );

            store = qMax( store - step, double( y ) );
        }
        else
            store = y;
    }

    Q_EMIT finished();
}
