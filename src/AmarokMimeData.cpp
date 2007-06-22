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

#include <QList>
#include <QUrl>

AmarokMimeData::AmarokMimeData()
    : QMimeData()
    , m_queryMaker( 0 )
    , m_deleteQueryMaker( true )
{
    //nothing to do
}

AmarokMimeData::~AmarokMimeData()
{
    if( m_deleteQueryMaker )
        delete m_queryMaker;
}

QStringList
AmarokMimeData::formats() const
{
    QStringList formats( QMimeData::formats() );
    if( !m_tracks.isEmpty() )
    {
        formats.append( "application/x-amarok-tracks" );
        if( !formats.contains( "text/uri-list" ) )
            formats.append( "text/uri-list" );
        if( !formats.contains( "text/plain" ) )
            formats.append( "text/plain" );
    }
    if( m_queryMaker )
        formats.append( "application/x-amarok-querymaker" );
    return formats;
}

bool
AmarokMimeData::hasFormat( const QString &mimeType ) const
{
    if( mimeType == "application/x-amarok-tracks" )
    {
        return !m_tracks.isEmpty();
    }
    else if( mimeType == "application/x-amarok-querymaker" )
    {
        return m_queryMaker;
    }
    else
        return QMimeData::hasFormat( mimeType );
}

Meta::TrackList
AmarokMimeData::tracks() const
{
    return m_tracks;
}

void
AmarokMimeData::setTracks( const Meta::TrackList &tracks )
{
    m_tracks = tracks;
}

QueryMaker*
AmarokMimeData::queryMaker()
{
    m_deleteQueryMaker = false;
    return m_queryMaker;
}

void
AmarokMimeData::setQueryMaker( QueryMaker *queryMaker )
{
    m_queryMaker = queryMaker;
}

QVariant
AmarokMimeData::retrieveData( const QString &mimeType, QVariant::Type type )
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

#include "AmarokMimeData.moc"

