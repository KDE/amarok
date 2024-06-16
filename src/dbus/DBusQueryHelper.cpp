/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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
 
#include "DBusQueryHelper.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h"
#include "core/support/Debug.h"

#include <QTimer>

Q_DECLARE_METATYPE( VariantMapList )

DBusQueryHelper::DBusQueryHelper( QObject *parent, Collections::QueryMaker *qm, const QDBusConnection &conn, const QDBusMessage &msg, bool mprisCompatible )
    : QObject( parent )
    , m_connection( conn )
    , m_message( msg )
    , m_mprisCompatibleResult( mprisCompatible )
    , m_timeout( false )
{
    qm->setAutoDelete( true );
    qm->setQueryType( Collections::QueryMaker::Track );
    connect( qm, &Collections::QueryMaker::newTracksReady, this, &DBusQueryHelper::slotResultReady, Qt::QueuedConnection );
    connect( qm, &Collections::QueryMaker::queryDone, this, &DBusQueryHelper::slotQueryDone, Qt::QueuedConnection );
    qm->run();

    //abort query after 15 seconds in case the query does not return
    QTimer::singleShot( 15000, this, &DBusQueryHelper::abortQuery );
}

void
DBusQueryHelper::slotResultReady( const Meta::TrackList &tracks )
{
    for( const Meta::TrackPtr &track : tracks )
    {
        if( m_mprisCompatibleResult )
            m_result.append( Meta::Field::mprisMapFromTrack( track ) );
        else
            m_result.append( Meta::Field::mapFromTrack( track ) );
    }
}

void
DBusQueryHelper::slotQueryDone()
{
    deleteLater();

    if( m_timeout )
        return;

    QDBusMessage reply = m_message.createReply( QVariant::fromValue( m_result ) );
    bool success = m_connection.send( reply );
    if( !success )
        debug() << "sending async reply failed";
}

void
DBusQueryHelper::abortQuery()
{
    deleteLater();
    m_timeout = true;

    QDBusMessage error = m_message.createErrorReply( QDBusError::InternalError, QStringLiteral("Internal timeout") );
    bool success = m_connection.send( error );
    if( !success )
        debug() << "sending async error failed";
}
