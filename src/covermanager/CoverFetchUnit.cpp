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

#include "CoverFetchUnit.h"
#include "Debug.h"

#include <QDomNodeList>
#include <QRegExp>
#include <QSet>

#define DEBUG_PREFIX "CoverFetchUnit"

/*
 * CoverFetchUnit
 */

CoverFetchUnit::CoverFetchUnit( Meta::AlbumPtr album,
                                const CoverFetchPayload *payload,
                                CoverFetch::Options opt )
    : QSharedData()
    , m_album( album )
    , m_options( opt )
    , m_payload( payload )
{
}

CoverFetchUnit::CoverFetchUnit( const CoverFetchSearchPayload *payload )
    : QSharedData()
    , m_options( CoverFetch::Interactive )
    , m_payload( payload )
{
}

CoverFetchUnit::CoverFetchUnit( const CoverFetchUnit &cpy )
    : QSharedData( cpy )
{
    m_album = cpy.m_album;
    m_options = cpy.m_options;

    switch( cpy.m_payload->type() )
    {
    case CoverFetchPayload::Info:
        m_payload = new CoverFetchInfoPayload( cpy.m_album );
        break;
    case CoverFetchPayload::Search:
        {
            const CoverFetchSearchPayload *payload =
                dynamic_cast< const CoverFetchSearchPayload * >( cpy.payload() );
            m_payload = new CoverFetchSearchPayload( payload->query() );
            break;
        }
    case CoverFetchPayload::Art:
        {
            const CoverFetchArtPayload *payload =
                dynamic_cast< const CoverFetchArtPayload * >( cpy.payload() );
            m_payload = new CoverFetchArtPayload( cpy.m_album, payload->isWild() );
            break;
        }
    default:
        m_payload = 0;
    }
}

CoverFetchUnit::~CoverFetchUnit()
{
    delete m_payload;
}

Meta::AlbumPtr
CoverFetchUnit::album() const
{
    return m_album;
}

const QStringList &
CoverFetchUnit::errors() const
{
    return m_errors;
}

CoverFetch::Options
CoverFetchUnit::options() const
{
    return m_options;
}

const CoverFetchPayload *
CoverFetchUnit::payload() const
{
    return m_payload;
}

bool
CoverFetchUnit::isInteractive() const
{
    bool interactive;
    switch( m_options )
    {
    case CoverFetch::Automatic:
        interactive = false;
        break;
    case CoverFetch::Interactive:
        interactive = true;
        break;
    }
    return interactive;
}

template< typename T >
void
CoverFetchUnit::addError( const T &error )
{
    m_errors << error;
}

CoverFetchUnit &CoverFetchUnit::operator=( const CoverFetchUnit &rhs )
{
    if( this == &rhs )
        return *this;

    switch( rhs.m_payload->type() )
    {
    case CoverFetchPayload::Info:
        m_payload = new CoverFetchInfoPayload( rhs.m_album );
        break;
    case CoverFetchPayload::Search:
        {
            const CoverFetchSearchPayload *payload =
                dynamic_cast< const CoverFetchSearchPayload * >( rhs.payload() );
            m_payload = new CoverFetchSearchPayload( payload->query() );
            break;
        }
    case CoverFetchPayload::Art:
        {
            const CoverFetchArtPayload *payload =
                dynamic_cast< const CoverFetchArtPayload * >( rhs.payload() );
            m_payload = new CoverFetchArtPayload( rhs.m_album, payload->isWild() );
        }
    default:
        m_payload = 0;
    }

    m_album = rhs.m_album;
    m_options = rhs.m_options;
    return *this;
}

bool CoverFetchUnit::operator==( const CoverFetchUnit &other ) const
{
    return m_album == other.m_album;
}

bool CoverFetchUnit::operator!=( const CoverFetchUnit &other ) const
{
    return !( *this == other );
}

#ifdef  DEBUG_PREFIX
# undef DEBUG_PREFIX
#endif
#define DEBUG_PREFIX "CoverFetchPayload"

/*
 * CoverFetchPayload
 */

CoverFetchPayload::CoverFetchPayload( const Meta::AlbumPtr album, CoverFetchPayload::Type type )
    : m_album( album )
    , m_method( ( type == Search ) ? QString( "album.search" )
                                   : album && album->hasAlbumArtist() ? QString( "album.getinfo" )
                                                                      : QString( "album.search" ) )
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
    : CoverFetchPayload( album, CoverFetchPayload::Info )
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
 * CoverFetchSearchPayload
 */

CoverFetchSearchPayload::CoverFetchSearchPayload( const QString &query )
    : CoverFetchPayload( Meta::AlbumPtr( 0 ), CoverFetchPayload::Search )
    , m_query( query )
{
    prepareUrls();
}

CoverFetchSearchPayload::~CoverFetchSearchPayload()
{
}

QString
CoverFetchSearchPayload::query() const
{
    return m_query;
}

void
CoverFetchSearchPayload::setQuery( const QString &query )
{
    m_query = query;
}

void
CoverFetchSearchPayload::prepareUrls()
{
    KUrl url;
    url.setScheme( "http" );
    url.setHost( "ws.audioscrobbler.com" );
    url.setPath( "/2.0/" );
    url.addQueryItem( "api_key", "402d3ca8e9bc9d3cf9b85e1202944ca5" );
    url.addQueryItem( "album", m_query );
    url.addQueryItem( "method", method() );
    m_urls.append( url );
}

/*
 * CoverFetchArtPayload
 */

CoverFetchArtPayload::CoverFetchArtPayload( const Meta::AlbumPtr album, bool wild )
    : CoverFetchPayload( album, CoverFetchPayload::Art )
    , m_wild( wild )
{
}

CoverFetchArtPayload::~CoverFetchArtPayload()
{
}

bool
CoverFetchArtPayload::isWild() const
{
    return m_wild;
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

        if( !m_wild && album() )
        {
            const Meta::TrackList tracks = album()->tracks();
            QStringList artistNames( "Various Artists" );
            foreach( const Meta::TrackPtr &track, tracks )
            {
                artistNames << track->artist()->name();
            }
            artistSet = normalize( artistNames ).toSet();
        }
    }
    else return;

    for( uint x = 0, len = results.length(); x < len; ++x )
    {
        const QDomNode albumNode = results.item( x );
        const QString artist = normalize( albumNode.namedItem( "artist" ).toElement().text() );

        if( searchMethod == "album.getinfo" && artist != albumArtist )
            continue;
        else if( searchMethod == "album.search" && !m_wild && !artistSet.contains( artist ) )
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

