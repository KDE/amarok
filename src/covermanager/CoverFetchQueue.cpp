/****************************************************************************************
 * Copyright (c) 2009 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#include "CoverFetchQueue.h"
#include "Debug.h"

#include <QDomNodeList>
#include <QRegExp>
#include <QSet>

CoverFetchQueue::CoverFetchQueue( QObject *parent )
    : QObject( parent )
{
}

CoverFetchQueue::~CoverFetchQueue()
{
}

bool
CoverFetchQueue::add( const CoverFetchUnit::Ptr unit )
{
    DEBUG_BLOCK
    if( m_queue.contains( unit ) )
    {
        m_queue.removeAll( unit );
    }
    m_queue.append( unit );
    emit fetchUnitAdded( unit );
    return true;
}

bool
CoverFetchQueue::add( const Meta::AlbumPtr album, bool interactive, const QByteArray &xml )
{
    CoverFetchPayload *payload;
    if( xml.isEmpty() )
    {
        payload = new CoverFetchInfoPayload( album );
    }
    else
    {
        CoverFetchArtPayload *art = new CoverFetchArtPayload( album );
        art->setXml( xml );
        payload = art;
    }
    return add( KSharedPtr< CoverFetchUnit >( new CoverFetchUnit( album, payload, interactive ) ) );
}

int
CoverFetchQueue::size() const
{
    return m_queue.size();
}

bool
CoverFetchQueue::isEmpty() const
{
    return m_queue.isEmpty();
}

void
CoverFetchQueue::clear()
{
    m_queue.clear();
}

void
CoverFetchQueue::remove( const CoverFetchUnit::Ptr unit )
{
    m_queue.removeAll( unit );
}

void
CoverFetchQueue::remove( const Meta::AlbumPtr album )
{
    m_queue.removeAt( index( album ) );
}

bool
CoverFetchQueue::contains( const Meta::AlbumPtr album ) const
{
    typedef QList< CoverFetchUnit::Ptr >::const_iterator ListIter;
    ListIter it   = m_queue.constBegin();
    ListIter last = m_queue.constEnd();
    while( it != last )
    {
        Meta::AlbumPtr t_album = (*it)->album();
        if( t_album == album )
            return true;
        ++it;
    }
    return false;
}

int
CoverFetchQueue::index( const Meta::AlbumPtr album ) const
{
    for( int i = 0, len = m_queue.size(); i < len; ++i )
    {
        if( m_queue.at( i )->album() == album )
            return i;
    }
    return -1;
}

const CoverFetchUnit::Ptr
CoverFetchQueue::take( const Meta::AlbumPtr album )
{
    for( int i = 0, end = this->size(); i < end; ++i )
    {
        const CoverFetchUnit::Ptr unit = m_queue.at( i );
        if( unit->album() == album )
        {
            m_queue.removeAt( i );
            return unit;
        }
    }
    // need to test if album exists with contains() first
    return KSharedPtr< CoverFetchUnit >();
}

/*
 * CoverFetchPayload
 */

CoverFetchPayload::CoverFetchPayload( const Meta::AlbumPtr album, CoverFetchPayload::Type type )
    : m_album( album )
    , m_method( album->hasAlbumArtist() ? QString( "album.getinfo" ) : QString( "album.search" ) )
    , m_type( type )
{
}

CoverFetchPayload::~CoverFetchPayload()
{
}

CoverFetchPayload::Type
CoverFetchPayload::type() const
{
    return m_type;
}

const KUrl::List &
CoverFetchPayload::urls() const
{
    return m_urls;
}

bool
CoverFetchPayload::isPrepared() const
{
    return !m_urls.isEmpty();
}

/*
 * CoverFetchInfoPayload
 */


CoverFetchInfoPayload::CoverFetchInfoPayload( const Meta::AlbumPtr album )
    : CoverFetchPayload( album, CoverFetchPayload::INFO )
{
    prepareUrls();
}

CoverFetchInfoPayload::~CoverFetchInfoPayload()
{
}

void
CoverFetchInfoPayload::prepareUrls()
{
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "album", album()->name() );

    if( album()->hasAlbumArtist() )
    {
        url.addQueryItem( "artist", album()->albumArtist()->name() );
    }
    url.addQueryItem( "method", method() );

    m_urls.append( url );
}

/*
 * CoverFetchArtPayload
 */

CoverFetchArtPayload::CoverFetchArtPayload( const Meta::AlbumPtr album )
    : CoverFetchPayload( album, CoverFetchPayload::ART )
{
}

CoverFetchArtPayload::~CoverFetchArtPayload()
{
}

void
CoverFetchArtPayload::setXml( const QByteArray &xml )
{
    m_xml = QString::fromUtf8( xml );
    prepareUrls();
}

void
CoverFetchArtPayload::prepareUrls()
{
    QDomDocument doc;
    if( !doc.setContent( m_xml ) )
    {
        debug() << "The xml obtained from Last.fm is invalid.";
        return;
    }

    QString albumArtist;
    QDomNodeList results;
    QSet< QString > artistSet;

    const QString &searchMethod = method();

    if( searchMethod == "album.getinfo" )
    {
        results = doc.documentElement().childNodes();
        albumArtist = normalize( album()->albumArtist()->name() );
    }
    else if( searchMethod == "album.search" )
    {
        results = doc.documentElement().namedItem( "results" ).namedItem( "albummatches" ).childNodes();

        const Meta::TrackList tracks = album()->tracks();
        QStringList artistNames( "Various Artists" );
        foreach( const Meta::TrackPtr &track, tracks )
        {
            artistNames << track->artist()->name();
        }
        artistSet = normalize( artistNames ).toSet();
    }
    else return;

    for( uint x = 0, len = results.length(); x < len; ++x )
    {
        const QDomNode albumNode = results.item( x );
        const QString artist = normalize( albumNode.namedItem( "artist" ).toElement().text() );

        if( searchMethod == "album.getinfo" && artist != albumArtist )
            continue;
        else if( searchMethod == "album.search" && !artistSet.contains( artist ) )
            continue;

        const QDomNodeList list = albumNode.childNodes();
        for( int i = 0, count = list.count(); i < count; ++i )
        {
            const QDomNode &node = list.item( i );
            if( node.nodeName() == "image" && node.hasAttributes() )
            {
                const QString imageSize = node.attributes().namedItem( "size" ).nodeValue();
                if( node.isElement() && imageSize == coverSize( ExtraLarge ) )
                {
                    const KUrl url( node.toElement().text() );
                    if( url.isValid() )
                        m_urls.append( url );
                }
            }
        }
    }
}

QString
CoverFetchArtPayload::coverSize( enum CoverSize size ) const
{
    QString str;
    switch( size )
    {
    case Small:  str = "small";      break;
    case Medium: str = "medium";     break;
    case Large:  str = "large";      break;
    default:     str = "extralarge"; break;
    }
    return str;
}

QString
CoverFetchArtPayload::normalize( const QString &raw )
{
    const QRegExp spaceRegExp  = QRegExp( "\\s" );
    return raw.toLower().remove( spaceRegExp ).normalized( QString::NormalizationForm_KC );
}

QStringList
CoverFetchArtPayload::normalize( const QStringList &rawList )
{
    QStringList cooked;
    foreach( const QString &raw, rawList )
    {
        cooked << normalize( raw );
    }
    return cooked;
}

#include "CoverFetchQueue.moc"
