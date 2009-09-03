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

#include "Debug.h"
#include "collection/QueryMaker.h"
#include "meta/MetaUtility.h"

Q_DECLARE_METATYPE( VariantMapList )
 
DBusQueryHelper::DBusQueryHelper( QObject *parent, QueryMaker *qm, const QDBusConnection &conn, const QDBusMessage &msg )
    : QObject( parent )
    , m_connection( conn )
    , m_message( msg )
{
    qm->setAutoDelete( true );
    qm->setQueryType( QueryMaker::Track );
    connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( slotResultReady( QString, Meta::TrackList ) ), Qt::QueuedConnection );
    connect( qm, SIGNAL( queryDone() ), this, SLOT( slotQueryDone() ), Qt::QueuedConnection );
    qm->run();
}

void
DBusQueryHelper::slotResultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    Q_UNUSED( collectionId );
    foreach( const Meta::TrackPtr &track, tracks )
    {
        m_result.append( Meta::Field::mapFromTrack( track ) );
    }
}

void
DBusQueryHelper::slotQueryDone()
{
    deleteLater();

    QDBusMessage reply = m_message.createReply( QVariant::fromValue( m_result ) );
    bool success = m_connection.send( reply );
    if( !success )
        debug() << "sending async reply failed";
}
