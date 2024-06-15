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

#define DEBUG_PREFIX "CoverFetchUnit"

#include "CoverFetchUnit.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QRegularExpression>
#include <QSet>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <KLocalizedString>

/*
 * CoverFetchUnit
 */

CoverFetchUnit::CoverFetchUnit( const Meta::AlbumPtr &album,
                                const CoverFetchPayload *payload,
                                CoverFetch::Option opt )
    : m_album( album )
    , m_options( opt )
    , m_payload( payload )
{
}

CoverFetchUnit::CoverFetchUnit( const CoverFetchPayload *payload, CoverFetch::Option opt )
    : m_album( payload->album() )
    , m_options( opt )
    , m_payload( payload )
{
}

CoverFetchUnit::CoverFetchUnit( const CoverFetchSearchPayload *payload )
    : m_album( payload->album() )
    , m_options( CoverFetch::WildInteractive )
    , m_payload( payload )
{
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

bool CoverFetchUnit::operator==( const CoverFetchUnit &other ) const
{
    return (m_album == other.m_album) && (m_options == other.m_options) && (m_payload == other.m_payload);
}

bool CoverFetchUnit::operator!=( const CoverFetchUnit &other ) const
{
    return !( *this == other );
}


/*
 * CoverFetchPayload
 */

CoverFetchPayload::CoverFetchPayload( const Meta::AlbumPtr &album,
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

Meta::AlbumPtr
CoverFetchPayload::album() const
{
    return m_album;
}

QString
CoverFetchPayload::sanitizeQuery( const QString &query )
{
    QString cooked( query );
    cooked.remove( QChar('?') );
    return cooked;
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

CoverFetchInfoPayload::CoverFetchInfoPayload( const Meta::AlbumPtr &album, const CoverFetch::Source src )
    : CoverFetchPayload( album, CoverFetchPayload::Info, src )
{
    prepareUrls();
}

CoverFetchInfoPayload::CoverFetchInfoPayload( const CoverFetch::Source src, const QByteArray &data )
    : CoverFetchPayload( Meta::AlbumPtr( nullptr ), CoverFetchPayload::Info, src )
{
    switch( src )
    {
    default:
        prepareUrls();
        break;
    case CoverFetch::Discogs:
        prepareDiscogsUrls( data );
        break;
    }
}

CoverFetchInfoPayload::~CoverFetchInfoPayload()
{
}

void
CoverFetchInfoPayload::prepareUrls()
{
    QUrl url;
    CoverFetch::Metadata metadata;

    switch( m_src )
    {
    default:
    case CoverFetch::LastFm:
        url.setScheme( "http" );
        url.setHost( "ws.audioscrobbler.com" );
        url.setPath( "/2.0/" );
        QUrlQuery query;
        query.addQueryItem( "api_key", Amarok::lastfmApiKey() );
        query.addQueryItem( "album", sanitizeQuery( album()->name() ) );

        if( album()->hasAlbumArtist() )
        {
            query.addQueryItem( "artist", sanitizeQuery( album()->albumArtist()->name() ) );
        }
        query.addQueryItem( "method", method() );
        url.setQuery( query );

        metadata[ "source" ] = "Last.fm";
        metadata[ "method" ] = method();
        break;
    }

    if( url.isValid() )
        m_urls.insert( url, metadata );
}

void
CoverFetchInfoPayload::prepareDiscogsUrls( const QByteArray &data )
{
    QXmlStreamReader xml( QString::fromUtf8(data) );
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( xml.isStartElement() && xml.name() == "searchresults" )
        {
            while( !xml.atEnd() && !xml.hasError() )
            {
                xml.readNext();
                const QStringRef &n = xml.name();
                if( xml.isEndElement() && n == "searchresults" )
                    break;
                if( !xml.isStartElement() )
                    continue;
                if( n == "result" )
                {
                    while( !xml.atEnd() && !xml.hasError() )
                    {
                        xml.readNext();
                        if( xml.isEndElement() && n == "result" )
                            break;
                        if( !xml.isStartElement() )
                            continue;
                        if( xml.name() == "uri" )
                        {
                            QUrl releaseUrl( xml.readElementText() );
                            QString releaseStr = releaseUrl.adjusted(QUrl::StripTrailingSlash).toString();
                            QString releaseId = releaseStr.split( QLatin1Char('/') ).last();

                            QUrl url;
                            url.setScheme( "http" );
                            url.setHost( "www.discogs.com" );
                            url.setPath( "/release/" + releaseId );
                            QUrlQuery query;
                            query.addQueryItem( "api_key", Amarok::discogsApiKey() );
                            query.addQueryItem( "f", "xml" );
                            url.setQuery( query );

                            CoverFetch::Metadata metadata;
                            metadata[ "source" ] = "Discogs";

                            if( url.isValid() )
                                m_urls.insert( url, metadata );
                        }
                        else
                            xml.skipCurrentElement();
                    }
                }
                else
                    xml.skipCurrentElement();
            }
        }
    }
}

/*
 * CoverFetchSearchPayload
 */

CoverFetchSearchPayload::CoverFetchSearchPayload( const QString &query,
                                                  const CoverFetch::Source src,
                                                  unsigned int page,
                                                  const Meta::AlbumPtr &album )
    : CoverFetchPayload( album, CoverFetchPayload::Search, src )
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
    QUrl url;
    QUrlQuery query;
    url.setScheme( "http" );
    CoverFetch::Metadata metadata;

    switch( m_src )
    {
    default:
    case CoverFetch::LastFm:
        url.setHost( "ws.audioscrobbler.com" );
        url.setPath( "/2.0/" );
        query.addQueryItem( "api_key", Amarok::lastfmApiKey() );
        query.addQueryItem( "limit", QString::number( 20 ) );
        query.addQueryItem( "page", QString::number( m_page ) );
        query.addQueryItem( "album", sanitizeQuery( m_query ) );
        query.addQueryItem( "method", method() );
        metadata[ "source" ] = "Last.fm";
        metadata[ "method" ] = method();
        break;

    case CoverFetch::Discogs:
        debug() << "Setting up a Discogs fetch";
        url.setHost( "www.discogs.com" );
        url.setPath( "/search" );
        query.addQueryItem( "api_key", Amarok::discogsApiKey() );
        query.addQueryItem( "page", QString::number( m_page + 1 ) );
        query.addQueryItem( "type", "all" );
        query.addQueryItem( "q", sanitizeQuery( m_query ) );
        query.addQueryItem( "f", "xml" );
        debug() << "Discogs Url: " << url;
        metadata[ "source" ] = "Discogs";
        break;

    case CoverFetch::Google:
        url.setHost( "images.google.com" );
        url.setPath( "/images" );
        query.addQueryItem( "q", sanitizeQuery( m_query ) );
        query.addQueryItem( "gbv", QChar( '1' ) );
        query.addQueryItem( "filter", QChar( '1' ) );
        query.addQueryItem( "start", QString::number( 20 * m_page ) );
        metadata[ "source" ] = "Google";
        break;
    }
    url.setQuery( query );
    debug() << "Fetching From URL: " << url;
    if( url.isValid() )
        m_urls.insert( url, metadata );
}

/*
 * CoverFetchArtPayload
 */

CoverFetchArtPayload::CoverFetchArtPayload( const Meta::AlbumPtr &album,
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
    : CoverFetchPayload( Meta::AlbumPtr( nullptr ), CoverFetchPayload::Art, src )
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
    if( m_src == CoverFetch::Google )
    {
        // google is special
        prepareGoogleUrls();
        return;
    }

    QXmlStreamReader xml( m_xml );
    xml.setNamespaceProcessing( false );
    switch( m_src )
    {
    default:
    case CoverFetch::LastFm:
        prepareLastFmUrls( xml );
        break;
    case CoverFetch::Discogs:
        prepareDiscogsUrls( xml );
        break;
    }

    if( xml.hasError() )
    {
        warning() << QString( "Error occurred when preparing %1 urls for %2: %3" )
            .arg( sourceString(), (album() ? album()->name() : "'unknown'"), xml.errorString() );
        debug() << urls();
    }
}

void
CoverFetchArtPayload::prepareDiscogsUrls( QXmlStreamReader &xml )
{
    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( !xml.isStartElement() || xml.name() != "release" )
            continue;

        const QString releaseId = xml.attributes().value( "id" ).toString();
        while( !xml.atEnd() && !xml.hasError() )
        {
            xml.readNext();
            const QStringRef &n = xml.name();
            if( xml.isEndElement() && n == "release" )
                break;
            if( !xml.isStartElement() )
                continue;

            CoverFetch::Metadata metadata;
            metadata[ "source" ] = "Discogs";
            if( n == "title" )
                metadata[ "title" ] = xml.readElementText();
            else if( n == "country" )
                metadata[ "country" ] = xml.readElementText();
            else if( n == "released" )
                metadata[ "released" ] = xml.readElementText();
            else if( n == "notes" )
                metadata[ "notes" ] = xml.readElementText();
            else if( n == "images" )
            {
                while( !xml.atEnd() && !xml.hasError() )
                {
                    xml.readNext();
                    if( xml.isEndElement() && xml.name() == "images" )
                        break;
                    if( !xml.isStartElement() )
                        continue;
                    if( xml.name() == "image" )
                    {
                        const QXmlStreamAttributes &attr = xml.attributes();
                        const QUrl thburl( attr.value( "uri150" ).toString() );
                        const QUrl uri( attr.value( "uri" ).toString() );
                        const QUrl url = (m_size == CoverFetch::ThumbSize) ? thburl : uri;
                        if( !url.isValid() )
                            continue;

                        metadata[ "releaseid"    ] = releaseId;
                        metadata[ "releaseurl"   ] = "http://discogs.com/release/" + releaseId;
                        metadata[ "normalarturl" ] = uri.url();
                        metadata[ "thumbarturl"  ] = thburl.url();
                        metadata[ "width"        ] = attr.value( "width"  ).toString();
                        metadata[ "height"       ] = attr.value( "height" ).toString();
                        metadata[ "type"         ] = attr.value( "type"   ).toString();
                        m_urls.insert( url, metadata );
                    }
                    else
                        xml.skipCurrentElement();
                }
            }
            else
                xml.skipCurrentElement();
        }
    }
}

void
CoverFetchArtPayload::prepareGoogleUrls()
{
    // code based on Audex CDDA Extractor
    /* Google backend 'temporarily' disabled in CoverFoundDialog.cpp, so comment out for now.
    // If this ever gets updated, needs to be ported to QRegularExpression
    QRegularExpression rx( "<a\\shref=\"(\\/imgres\\?imgurl=[^\"]+)\">[\\s\\n]*<img[^>]+src=\"([^\"]+)\"" );
    rx.setCaseSensitivity( Qt::CaseInsensitive );
    rx.setMinimal( true );

    int pos = 0;
    QString html = m_xml.replace( QLatin1String("&amp;"), QLatin1String("&") );

    while( ( (pos = rx.indexIn( html, pos ) ) != -1 ) )
    {
        QUrl url( "http://www.google.com" + rx.cap( 1 ) );
        QUrlQuery query( url.query() );

        CoverFetch::Metadata metadata;
        metadata[ "width" ] = query.queryItemValue( "w" );
        metadata[ "height" ] = query.queryItemValue( "h" );
        metadata[ "size" ] = query.queryItemValue( "sz" );
        metadata[ "imgrefurl" ] = query.queryItemValue( "imgrefurl" );
        metadata[ "normalarturl" ] = query.queryItemValue("imgurl");
        metadata[ "source" ] = "Google";

        if( !rx.cap( 2 ).isEmpty() )
            metadata[ "thumbarturl" ] = rx.cap( 2 );

        url.clear();
        switch( m_size )
        {
        default:
        case CoverFetch::ThumbSize:
            url = QUrl( metadata.value( "thumbarturl" ) );
            break;
        case CoverFetch::NormalSize:
            url = QUrl( metadata.value( "normalarturl" ) );
            break;
        }

        if( url.isValid() )
            m_urls.insert( url, metadata );

        pos += rx.matchedLength();
    } */
}

void
CoverFetchArtPayload::prepareLastFmUrls( QXmlStreamReader &xml )
{
    QSet<QString> artistSet;
    if( method() == "album.getinfo" )
    {
        artistSet << normalize( ( album() && album()->albumArtist() )
                                ? album()->albumArtist()->name()
                                : i18n( "Unknown Artist" ) );
    }
    else if( method() == "album.search" )
    {
        if( !m_wild && album() )
        {
            const Meta::TrackList tracks = album()->tracks();
            QStringList artistNames( "Various Artists" );
            foreach( const Meta::TrackPtr &track, tracks )
                artistNames << ( track->artist() ? track->artist()->name()
                                                 : i18n( "Unknown Artist" ) );
            QStringList artistNamesNormalized = normalize( artistNames );
            QSet<QString> addArtistSet(artistNamesNormalized.begin(), artistNamesNormalized.end());
            artistSet += addArtistSet;
        }
    }
    else return;

    while( !xml.atEnd() && !xml.hasError() )
    {
        xml.readNext();
        if( !xml.isStartElement() || xml.name() != "album" )
            continue;

        QHash<QString, QString> coverUrlHash;
        CoverFetch::Metadata metadata;
        metadata[ "source" ] = "Last.fm";
        while( !xml.atEnd() && !xml.hasError() )
        {
            xml.readNext();
            const QStringRef &n = xml.name();
            if( xml.isEndElement() && n == "album" )
                break;
            if( !xml.isStartElement() )
                continue;

            if( n == "name" )
            {
                metadata[ "name" ] = xml.readElementText();
            }
            else if( n == "artist" )
            {
                const QString &artist = xml.readElementText();
                if( !artistSet.contains( artist ) )
                    continue;
                metadata[ "artist" ] = artist;
            }
            else if( n == "url" )
            {
                metadata[ "releaseurl" ] = xml.readElementText();
            }
            else if( n == "image" )
            {
                QString sizeStr = xml.attributes().value("size").toString();
                coverUrlHash[ sizeStr ] = xml.readElementText();
            }
        }

        QStringList acceptableSizes;
        acceptableSizes << "large" << "medium" << "small";
        metadata[ "thumbarturl" ] = firstAvailableValue( acceptableSizes, coverUrlHash );

        acceptableSizes.clear();
        acceptableSizes << "extralarge" << "large";
        metadata[ "normalarturl" ] = firstAvailableValue( acceptableSizes, coverUrlHash );

        QUrl url( m_size == CoverFetch::ThumbSize ? metadata["thumbarturl"] : metadata["normalarturl"] );
        if( url.isValid() )
            m_urls.insert( url , metadata );
    }
}

QString
CoverFetchArtPayload::firstAvailableValue( const QStringList &keys, const QHash<QString, QString> &hash )
{
    for( int i = 0, size = keys.size(); i < size; ++i )
    {
        QString value( hash.value( keys.at(i) ) );
        if( !value.isEmpty() )
            return value;
    }
    return QString();
}

QString
CoverFetchArtPayload::normalize( const QString &raw )
{
    const QRegularExpression spaceRegExp  = QRegularExpression( "\\s" );
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

