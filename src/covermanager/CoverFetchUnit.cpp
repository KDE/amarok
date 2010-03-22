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

#include "Amarok.h"
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
                                CoverFetch::Option opt )
    : QSharedData()
    , m_album( album )
    , m_options( opt )
    , m_payload( payload )
{
}

CoverFetchUnit::CoverFetchUnit( const CoverFetchPayload *payload, CoverFetch::Option opt )
    : QSharedData()
    , m_album( Meta::AlbumPtr( 0 ) )
    , m_options( opt )
    , m_payload( payload )
{
}

CoverFetchUnit::CoverFetchUnit( const CoverFetchSearchPayload *payload )
    : QSharedData()
    , m_options( CoverFetch::WildInteractive )
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
        {
            m_payload = new CoverFetchInfoPayload( cpy.m_album, cpy.m_payload->source() );
            break;
        }
        case CoverFetchPayload::Search:
        {
            typedef CoverFetchSearchPayload CFSP;
            const CFSP *payload = dynamic_cast< const CFSP* >( cpy.payload() );
            m_payload = new CoverFetchSearchPayload( payload->query() );
            break;
        }
        case CoverFetchPayload::Art:
        {
            typedef CoverFetchArtPayload CFAP;
            const CFAP *payload = dynamic_cast< const CFAP* >( cpy.payload() );
            m_payload = new CoverFetchArtPayload( cpy.m_album,
                                                  payload->imageSize(),
                                                  payload->source(),
                                                  payload->isWild() );
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

CoverFetch::Option
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
    bool interactive( false );
    switch( m_options )
    {
    case CoverFetch::Automatic:
        interactive = false;
        break;
    case CoverFetch::Interactive:
    case CoverFetch::WildInteractive:
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
        {
            m_payload = new CoverFetchInfoPayload( rhs.m_album, rhs.payload()->source() );
            break;
        }
        case CoverFetchPayload::Search:
        {
            typedef CoverFetchSearchPayload CFSP;
            const CFSP *payload = dynamic_cast< const CFSP* >( rhs.payload() );
            m_payload = new CoverFetchSearchPayload( payload->query() );
            break;
        }
        case CoverFetchPayload::Art:
        {
            typedef CoverFetchArtPayload CFAP;
            const CFAP *payload = dynamic_cast< const CFAP* >( rhs.payload() );
            m_payload = new CoverFetchArtPayload( rhs.m_album,
                                                  payload->imageSize(),
                                                  payload->source(),
                                                  payload->isWild() );
            break;
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

CoverFetchPayload::CoverFetchPayload( const Meta::AlbumPtr album,
                                      CoverFetchPayload::Type type,
                                      CoverFetch::Source src )
    : m_src( src )
    , m_album( album )
    , m_method( ( type == Search ) ? QString( "album.search" )
                                   : album && album->hasAlbumArtist() ? QString( "album.getinfo" )
                                                                      : QString( "album.search" ) )
    , m_type( type )
{
}

CoverFetchPayload::~CoverFetchPayload()
{
}

CoverFetch::Source
CoverFetchPayload::source() const
{
    return m_src;
}

CoverFetchPayload::Type
CoverFetchPayload::type() const
{
    return m_type;
}

const CoverFetch::Urls &
CoverFetchPayload::urls() const
{
    return m_urls;
}

const QString
CoverFetchPayload::sourceString() const
{
    QString source;
    switch( m_src )
    {
    case CoverFetch::LastFm:
        source = "Last.fm";
        break;
    case CoverFetch::Yahoo:
        source = "Yahoo!";
        break;
    case CoverFetch::Google:
        source = "Google";
        break;
    case CoverFetch::Discogs:
        source = "Discogs";
        break;
    default:
        source = "Unknown";
    }
    return source;
}

bool
CoverFetchPayload::isPrepared() const
{
    return !m_urls.isEmpty();
}

/*
 * CoverFetchInfoPayload
 */

CoverFetchInfoPayload::CoverFetchInfoPayload( const Meta::AlbumPtr album, const CoverFetch::Source src )
    : CoverFetchPayload( album, CoverFetchPayload::Info, src )
{
    prepareUrls();
}

CoverFetchInfoPayload::CoverFetchInfoPayload( const CoverFetch::Source src, const QByteArray &xml )
    : CoverFetchPayload( Meta::AlbumPtr( 0 ), CoverFetchPayload::Info, src )
    , m_xml( QString::fromUtf8( xml ) )
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
    CoverFetch::Metadata metadata;

    switch( m_src )
    {
    default:
    case CoverFetch::LastFm:
        url.setScheme( "http" );
        url.setHost( "ws.audioscrobbler.com" );
        url.setPath( "/2.0/" );
        url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
        url.addQueryItem( "album", album()->name() );

        if( album()->hasAlbumArtist() )
        {
            url.addQueryItem( "artist", album()->albumArtist()->name() );
        }
        url.addQueryItem( "method", method() );

        metadata[ "source" ] = "Last.fm";
        metadata[ "method" ] = method();
        break;

    case CoverFetch::Discogs:
        QDomDocument doc;
        if( doc.setContent( m_xml ) )
            prepareDiscogsUrls( doc );
        break;
    }
    m_urls.insert( url, metadata );
}

void
CoverFetchInfoPayload::prepareDiscogsUrls( const QDomDocument &doc )
{
    const QDomElement &docElem = doc.documentElement();

    const QDomNode &respNode = docElem.namedItem( "resp" );
    if( respNode.hasAttributes() )
    {
        const QString &stat = respNode.attributes().namedItem( "stat" ).nodeValue();
        if( stat != "ok" )
            return;
    }

    const QDomNodeList &results = docElem.namedItem( "searchresults" ).childNodes();
    for( uint x = 0, len = results.length(); x < len; ++x )
    {
        const QDomNode resultNode = results.item( x );
        const KUrl releaseUrl     = resultNode.namedItem( "uri" ).toElement().text();
        const QString releaseStr  = releaseUrl.url( KUrl::RemoveTrailingSlash );
        const QString releaseId   = releaseStr.split( '/' ).last();

        KUrl url;
        url.setScheme( "http" );
        url.setHost( "www.discogs.com" );
        url.setPath( "/release/" + releaseId );
        url.addQueryItem( "api_key", Amarok::discogsApiKey() );
        url.addQueryItem( "f", "xml" );

        if( !url.isValid() )
            continue;

        CoverFetch::Metadata metadata;
        m_urls.insert( url, metadata );
    }
}

/*
 * CoverFetchSearchPayload
 */

CoverFetchSearchPayload::CoverFetchSearchPayload( const QString &query,
                                                  const CoverFetch::Source src,
                                                  unsigned int page )
    : CoverFetchPayload( Meta::AlbumPtr( 0 ), CoverFetchPayload::Search, src )
    , m_page( page )
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
CoverFetchSearchPayload::prepareUrls()
{
    KUrl url;
    url.setScheme( "http" );
    CoverFetch::Metadata metadata;

    switch( m_src )
    {
    default:
    case CoverFetch::LastFm:
        url.setHost( "ws.audioscrobbler.com" );
        url.setPath( "/2.0/" );
        url.addQueryItem( "api_key", Amarok::lastfmApiKey() );
        url.addQueryItem( "limit", QString::number( 20 ) );
        url.addQueryItem( "page", QString::number( m_page ) );
        url.addQueryItem( "album", m_query );
        url.addQueryItem( "method", method() );
        metadata[ "source" ] = "Last.fm";
        metadata[ "method" ] = method();
        break;

    case CoverFetch::Discogs:
        url.setHost( "www.discogs.com" );
        url.setPath( "/search" );
        url.addQueryItem( "api_key", Amarok::discogsApiKey() );
        url.addQueryItem( "page", QString::number( m_page + 1 ) );
        url.addQueryItem( "type", "all" );
        url.addQueryItem( "q", m_query );
        url.addQueryItem( "f", "xml" );
        metadata[ "source" ] = "Discogs";
        break;

    case CoverFetch::Yahoo:
        url.setHost( "boss.yahooapis.com" );
        url.setPath( "/ysearch/images/v1/" + m_query );
        url.addQueryItem( "appid", Amarok::yahooBossApiKey() );
        url.addQueryItem( "count", QString::number( 20 ) );
        url.addQueryItem( "start", QString::number( 20 * m_page ) );
        url.addQueryItem( "format", "xml" );
        metadata[ "source" ] = "Yahoo!";
        break;

    case CoverFetch::Google:
        url.setHost( "images.google.com" );
        url.setPath( "/images" );
        url.addQueryItem( "q", m_query );
        url.addQueryItem( "gbv", QChar( '1' ) );
        url.addQueryItem( "filter", QChar( '1' ) );
        url.addQueryItem( "start", QString::number( 20 * m_page ) );
        metadata[ "source" ] = "Google";
        break;
    }

    m_urls.insert( url, metadata );
}

/*
 * CoverFetchArtPayload
 */

CoverFetchArtPayload::CoverFetchArtPayload( const Meta::AlbumPtr album,
                                            const CoverFetch::ImageSize size,
                                            const CoverFetch::Source src,
                                            bool wild )
    : CoverFetchPayload( album, CoverFetchPayload::Art, src )
    , m_size( size )
    , m_wild( wild )
{
}

CoverFetchArtPayload::CoverFetchArtPayload( const CoverFetch::ImageSize size,
                                            const CoverFetch::Source src,
                                            bool wild )
    : CoverFetchPayload( Meta::AlbumPtr( 0 ), CoverFetchPayload::Art, src )
    , m_size( size )
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

CoverFetch::ImageSize
CoverFetchArtPayload::imageSize() const
{
    return m_size;
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
    if( ( m_src != CoverFetch::Google ) && !doc.setContent( m_xml ) )
    {
        debug() << QString( "The xml obtained from %1 is invalid." ).arg( sourceString() );
        return;
    }

    switch( m_src )
    {
    case CoverFetch::LastFm:
        prepareLastFmUrls( doc );
        break;
    case CoverFetch::Yahoo:
        prepareYahooUrls( doc );
        break;
    case CoverFetch::Google:
        prepareGoogleUrls( m_xml );
        break;
    case CoverFetch::Discogs:
        prepareDiscogsUrls( doc );
        break;
    }
}

void
CoverFetchArtPayload::prepareDiscogsUrls( const QDomDocument &doc )
{
    const QDomElement &docElem = doc.documentElement();
    const QDomNode &respNode = docElem.namedItem( "resp" );
    if( respNode.hasAttributes() )
    {
        const QString &stat = respNode.attributes().namedItem( "stat" ).nodeValue();
        if( stat != "ok" )
            return;
    }

    CoverFetch::Metadata metadata;
    const QDomNode &releaseNode = docElem.namedItem( "release" );
    if( releaseNode.hasAttributes() )
    {
        const QDomNamedNodeMap attr = releaseNode.attributes();
        metadata[ "releaseid" ] = attr.namedItem( "id" ).nodeValue();
    }

    // TODO: there are a lot more discogs info that can be extracted as metadata.
    metadata[ "notes" ] = releaseNode.namedItem( "notes" ).toElement().text();
    metadata[ "title" ] = releaseNode.namedItem( "title" ).toElement().text();
    metadata[ "country" ] = releaseNode.namedItem( "country" ).toElement().text();
    metadata[ "released" ] = releaseNode.namedItem( "released" ).toElement().text();
    metadata[ "source" ] = "Discogs";

    const QDomNodeList imageNodes = releaseNode.namedItem( "images" ).childNodes();
    for( uint x = 0, len = imageNodes.length(); x < len; ++x )
    {
        const QDomNode &node = imageNodes.item( x );
        if( node.hasAttributes() )
        {
            const QDomNamedNodeMap attr = node.attributes();
            const QString thburl = attr.namedItem( "uri150" ).nodeValue();
            const KUrl uri       = attr.namedItem( "uri"    ).nodeValue();

            KUrl url;
            switch( m_size )
            {
            case CoverFetch::NormalSize:
                url = uri;
                break;
            case CoverFetch::ThumbSize:
            default:
                url = KUrl( thburl );
                break;
            }

            if( !url.isValid() )
                continue;

            const QString height = attr.namedItem( "height" ).nodeValue();
            const QString width  = attr.namedItem( "width"  ).nodeValue();
            const QString type   = attr.namedItem( "type"   ).nodeValue();
            const QString relUrl = "http://discogs.com/release/" + metadata.value( "releaseid" );

            metadata[ "releaseurl"   ] = relUrl;
            metadata[ "normalarturl" ] = uri.url();
            metadata[ "thumbarturl"  ] = thburl;
            metadata[ "width"        ] = width;
            metadata[ "height"       ] = height;
            metadata[ "type"         ] = type;

            m_urls.insert( url, metadata );
        }
    }
}

void
CoverFetchArtPayload::prepareGoogleUrls( const QString &html )
{
    // code based on Audex CDDA Extractor
    QRegExp rx( "<a\\shref=(\\/imgres\\?imgurl=[a-zA-Z0-9\\&\\_\\%\\/\\=\\.\\:\\-\\?\\,\\(\\)]+)>[\\s\\n]*<img\\ssrc=([a-zA-Z0-9\\&\\_\\%\\/\\=\\.\\:\\-\\?\\,\\(\\)]+).*>[\\s\\n]*</a>" );
    rx.setMinimal( true );

    int pos = 0;
    while( ( (pos = rx.indexIn( html, pos ) ) != -1 ) )
    {
        KUrl url( "http://www.google.com" + rx.cap( 1 ) );

        CoverFetch::Metadata metadata;
        metadata[ "width" ] = url.queryItemValue( "w" );
        metadata[ "height" ] = url.queryItemValue( "h" );
        metadata[ "size" ] = url.queryItemValue( "sz" );
        metadata[ "imgrefurl" ] = url.queryItemValue( "imgrefurl" );
        metadata[ "normalarturl" ] = url.queryItemValue("imgurl");
        metadata[ "source" ] = "Google";

        if( !rx.cap( 2 ).isEmpty() )
            metadata[ "thumbarturl" ] = rx.cap( 2 );

        switch( m_size )
        {
        case CoverFetch::NormalSize:
            url = metadata.value( "normalarturl" );
            break;
        case CoverFetch::ThumbSize:
        default:
            url = metadata.value( "thumbarturl" );
            break;
        }

        if( url.isValid() )
            m_urls.insert( url, metadata );

        pos += rx.matchedLength();
    }
}

void
CoverFetchArtPayload::prepareLastFmUrls( const QDomDocument &doc )
{
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

        KUrl url;
        CoverFetch::Metadata metadata;
        metadata[ "source" ] = "Last.fm";

        const QDomNodeList list = albumNode.childNodes();
        for( int i = 0, count = list.count(); i < count; ++i )
        {
            const QDomNode &node = list.item( i );
            if( node.nodeName() == "image" && node.hasAttributes() )
            {
                if( !node.isElement() )
                    continue;

                const QString elementText = node.toElement().text();
                const QString sizeStr = node.attributes().namedItem( "size" ).nodeValue();
                enum CoverFetch::ImageSize imageSize = str2CoverSize( sizeStr );

                switch( imageSize )
                {
                case CoverFetch::ThumbSize:
                    metadata[ "thumbarturl" ] = elementText;
                    break;
                case CoverFetch::NormalSize:
                    metadata[ "normalarturl" ] = elementText;
                    break;
                }

                if( sizeStr == coverSize2str( m_size ) )
                {
                    url = elementText;
                }
            }
        }

        if( !url.isValid() )
            continue;

        QStringList tags;
        tags << "name" << "artist";
        foreach( const QString &tag, tags )
        {
            const QDomElement e = albumNode.namedItem( tag ).toElement();
            if( !e.isNull() )
                metadata[ tag ] = e.text();
        }

        const QDomElement urlElem = albumNode.namedItem( "url" ).toElement();
        if( !urlElem.isNull() )
            metadata[ "releaseurl" ] = urlElem.text();

        m_urls.insert( url, metadata );
    }
}

void
CoverFetchArtPayload::prepareYahooUrls( const QDomDocument &doc )
{
    const QDomNodeList results = doc.documentElement().namedItem( "resultset_images" ).childNodes();
    for( uint x = 0, len = results.length(); x < len; ++x )
    {
        const QDomNode albumNode = results.item( x );
        const KUrl url = albumNode.namedItem( coverSize2str( imageSize() ) ).toElement().text();
        if( !url.isValid() )
            continue;

        CoverFetch::Metadata metadata;
        metadata[ "source" ] = "Yahoo!";
        metadata[ "thumbarturl" ] = albumNode.namedItem( "thumbnail_url" ).toElement().text();
        metadata[ "normalarturl" ] = albumNode.namedItem( "url" ).toElement().text();

        const QDomNodeList list = albumNode.childNodes();
        for( int i = 0, count = list.count(); i < count; ++i )
        {
            const QDomNode &node = list.item( i );
            if( node.isElement() )
            {
                const QDomElement element = node.toElement();
                const QString elementText = element.text();
                const QString elementTag  = element.tagName();
                metadata[ elementTag ] = elementText;
            }
        }

        if( metadata.contains( "abstract" ) )
        {
            const QString notes = metadata.take( "abstract" );
            metadata[ "notes" ] = notes;
        }
        m_urls.insert( url, metadata );
    }
}

QString
CoverFetchArtPayload::coverSize2str( enum CoverFetch::ImageSize size ) const
{
    QString str;
    switch( size )
    {
    case CoverFetch::ThumbSize:
        if( m_src == CoverFetch::LastFm )
            str = "large"; // around 128x128
        else if( m_src == CoverFetch::Yahoo )
            str = "thumbnail_url";
        break;
    case CoverFetch::NormalSize:
    default:
        if( m_src == CoverFetch::LastFm )
            str = "extralarge"; // up to 300x300
        else if( m_src == CoverFetch::Yahoo )
            str = "url";
        break;
    }
    return str;
}

enum CoverFetch::ImageSize
CoverFetchArtPayload::str2CoverSize( const QString &string ) const
{
    enum CoverFetch::ImageSize size;
    if( string == "large" || string == "thumbnail_url" )
        size = CoverFetch::ThumbSize;
    else
        size = CoverFetch::NormalSize;
    return size;
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

