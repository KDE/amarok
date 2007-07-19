/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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

#include "AmarokMimeData.h"

#include "debug.h"

#include <QCoreApplication>
#include <QList>
#include <QUrl>

const QString AmarokMimeData::TRACK_MIME = "application/x-amarok-tracks";

AmarokMimeData::AmarokMimeData()
    : QMimeData()
    , m_deleteQueryMakers( true )
    , m_completedQueries( 0 )
{
    //nothing to do
}

AmarokMimeData::~AmarokMimeData()
{
    if( m_deleteQueryMakers )
        qDeleteAll( m_queryMakers );
}

QStringList
AmarokMimeData::formats() const
{
    DEBUG_BLOCK
    QStringList formats( QMimeData::formats() );
    if( !m_tracks.isEmpty() || !m_queryMakers.isEmpty())
    {
        formats.append( TRACK_MIME );
        if( !formats.contains( "text/uri-list" ) )
            formats.append( "text/uri-list" );
        if( !formats.contains( "text/plain" ) )
            formats.append( "text/plain" );
    }
    return formats;
}

bool
AmarokMimeData::hasFormat( const QString &mimeType ) const
{
    DEBUG_BLOCK
   
    if( mimeType == TRACK_MIME )
    {
        debug() << "yep it has " << TRACK_MIME << endl;
        //FIXME m_tracks is always empty
        //return !m_tracks.isEmpty() || !m_queryMakers.isEmpty().;
        return true;
    }
    else
    {
         debug() << "not x-amarok" << " mimeType is " << mimeType << endl;
        return QMimeData::hasFormat( mimeType );
    }
}

Meta::TrackList
AmarokMimeData::tracks() const
{
    while( m_completedQueries < m_queryMakers.count() )
    {
        QCoreApplication::instance()->processEvents( QEventLoop::AllEvents );
    }
    return m_tracks;
}

void
AmarokMimeData::setTracks( const Meta::TrackList &tracks )
{
    m_tracks = tracks;
}

QList<QueryMaker*>
AmarokMimeData::queryMakers()
{
    m_deleteQueryMakers = false;
    return m_queryMakers;
}

void
AmarokMimeData::addQueryMaker( QueryMaker *queryMaker )
{
    m_queryMakers.append( queryMaker );
}

void
AmarokMimeData::setQueryMakers( const QList<QueryMaker*> &queryMakers )
{
    m_queryMakers << queryMakers;
}

QVariant
AmarokMimeData::retrieveData( const QString &mimeType, QVariant::Type type ) const
{
    if( !m_tracks.isEmpty() )
    {
        if( mimeType == "text/uri-list" && type == QVariant::List )
        {
            QList<QVariant> list;
            foreach( Meta::TrackPtr track, m_tracks )
            {
                list.append( QVariant( QUrl( track->playableUrl().url() ) ) );
            }
            return QVariant( list );
        }
        if( mimeType == "text/plain" && type == QVariant::String )
        {
            QString result;
            foreach( Meta::TrackPtr track, m_tracks )
            {
                if( !result.isEmpty() )
                    result += '\n';
                result += track->artist()->prettyName();
                result += " - ";
                result += track->prettyName();
            }
            return QVariant( result );
        }
    }
    return QMimeData::retrieveData( mimeType, type );
}

void
AmarokMimeData::startQueries()
{
    DEBUG_BLOCK
    foreach( QueryMaker *qm, m_queryMakers )
    {
        qm->startTrackQuery();
        connect( qm, SIGNAL( newResultReady( QString, Meta::TrackList ) ), this, SLOT( newResultReady( QString, Meta::TrackList ) ), Qt::QueuedConnection );
        connect( qm, SIGNAL( queryDone() ), this, SLOT( queryDone() ), Qt::QueuedConnection );
        qm->run();
    }
}

void
AmarokMimeData::newResultReady( const QString &collectionId, const Meta::TrackList &tracks )
{
    Q_UNUSED( collectionId )
    //TODO: sort results
    m_tracks << tracks;
}

void
AmarokMimeData::queryDone()
{
    m_completedQueries++;
}

#include "AmarokMimeData.moc"

