/*
 *  Copyright 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
 
#include "DBusQueryHelper.h"
 
#include "collection/QueryMaker.h"
#include "meta/MetaUtility.h"
 
DBusQueryHelper::DBusQueryHelper( QObject *parent, QueryMaker *qm, const QString &token )
    : QObject( parent )
    , m_token( token )
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
    emit queryResult( m_token, m_result );
}
