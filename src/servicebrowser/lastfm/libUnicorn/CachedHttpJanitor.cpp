/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "CachedHttpJanitor.h"
#include "Settings.h"

#include <QDir>
#include <QDebug>
#include <QDateTime>


CachedHttpJanitor::CachedHttpJanitor( const QString& cacheDir, QObject* parent ) :
    QThread( parent ),
    m_cacheDir( cacheDir ),
    m_abort( false )
{
    start();
}


CachedHttpJanitor::~CachedHttpJanitor()
{
    abort();

    qDebug() << "Waiting on CachedHttpJanitor thread!";
    wait( 5000 );
    qDebug() << "Waiting on CachedHttpJanitor finished!";
}


void
CachedHttpJanitor::run()
{
    QDir d( m_cacheDir );
    const QStringList files = d.entryList( QStringList() << "c*", QDir::Files );

    foreach( QString file, files )
    {
        if ( m_abort )
            return;

        QFile f( d.filePath( file ) );
        if ( f.open( QIODevice::ReadOnly ) )
        {
            QByteArray expdate = f.read( 10 );
            f.close();

            bool ok;
            if ( expdate.toUInt( &ok ) < QDateTime::currentDateTime().toTime_t() )
            {
                if ( !ok )
                    qDebug() << "Doesn't seem to be a valid cache file:" << d.filePath( file );

                if ( d.remove( file ) )
                    qDebug() << "Removing from cache:" << d.filePath( file );
                else
                    qDebug() << "Failed removing from cache:" << d.filePath( file );
            }
        }
    }

    deleteLater();
}
