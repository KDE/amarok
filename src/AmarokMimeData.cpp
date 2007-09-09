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

class AmarokMimeData::Private
{
public:
    Private() : deleteQueryMakers( true ), completedQueries( 0 )
    {}

    ~Private()
    {
        if( deleteQueryMakers )
            qDeleteAll( queryMakers );
    }

    Meta::TrackList tracks;
    QList<QueryMaker*> queryMakers;
    QMap<QueryMaker*, Meta::TrackList> trackMap;
    bool deleteQueryMakers;
    int completedQueries;

};

AmarokMimeData::AmarokMimeData()
    : QMimeData()
    , d( new Private() )
{
    //nothing to do
}

AmarokMimeData::~AmarokMimeData()
{
    delete d;
}

QStringList
AmarokMimeData::formats() const
{
    QStringList formats( QMimeData::formats() );
    if( !d->tracks.isEmpty() || !d->queryMakers.isEmpty())
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
    if( mimeType == TRACK_MIME || mimeType == "text/uri-list" || mimeType == "text/plain" )
        return !d->tracks.isEmpty() || !d->queryMakers.isEmpty();
    else
        return QMimeData::hasFormat( mimeType );
}

Meta::TrackList
AmarokMimeData::tracks() const
{
    while( d->completedQueries < d->queryMakers.count() )
    {
        QCoreApplication::instance()->processEvents( QEventLoop::AllEvents );
    }
    Meta::TrackList result = d->tracks;
    foreach( QueryMaker *qm, d->queryMakers )
    {
        if( d->trackMap.contains( qm ) )
            result << d->trackMap.value( qm );
    }
    return result;
}

void
AmarokMimeData::setTracks( const Meta::TrackList &tracks )
{
    d->tracks = tracks;
}

void
AmarokMimeData::addTracks( const Meta::TrackList &tracks )
{
    d->tracks << tracks;
}

QList<QueryMaker*>
AmarokMimeData::queryMakers()
{
    d->deleteQueryMakers = false;
    return d->queryMakers;
}

void
AmarokMimeData::addQueryMaker( QueryMaker *queryMaker )
{
    d->queryMakers.append( queryMaker );
}

void
AmarokMimeData::setQueryMakers( const QList<QueryMaker*> &queryMakers )
{
    d->queryMakers << queryMakers;
}

QVariant
AmarokMimeData::retrieveData( const QString &mimeType, QVariant::Type type ) const
{
    Meta::TrackList tracks = this->tracks();
    if( !tracks.isEmpty() )
    {
        if( mimeType == "text/uri-list" && type == QVariant::List )
        {
            QList<QVariant> list;
            foreach( Meta::TrackPtr track, tracks )
            {
                list.append( QVariant( QUrl( track->playableUrl().url() ) ) );
            }
            return QVariant( list );
        }
        if( mimeType == "text/plain" && type == QVariant::String )
        {
            QString result;
            foreach( Meta::TrackPtr track, tracks )
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
    foreach( QueryMaker *qm, d->queryMakers )
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
    QueryMaker *qm = dynamic_cast<QueryMaker*>( sender() );
    if( qm )
    {
        d->trackMap.insert( qm, tracks );
    }
    else
        d->tracks << tracks;
}

void
AmarokMimeData::queryDone()
{
    d->completedQueries++;
}

#include "AmarokMimeData.moc"

