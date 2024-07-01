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
    , m_method( ( type == Search ) ? QStringLiteral( "album.search" )
                                   : album && album->hasAlbumArtist() ? QStringLiteral( "album.getinfo" )
                                                                      : QStringLiteral( "album.search" ) )
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
        source = QStringLiteral("Last.fm");
        break;
    case CoverFetch::Google:
        source = QStringLiteral("Google");
        break;
    case CoverFetch::Discogs:
        source = QStringLiteral("Discogs");
        break;
    default:
        source = QStringLiteral("Unknown");
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
        url.setScheme( QStringLiteral("https") );
        url.setHost( QStringLiteral("ws.audioscrobbler.com") );
        url.setPath( QStringLiteral("/2.0/") );
        QUrlQuery query;
        query.addQueryItem( QStringLiteral("api_key"), QLatin1String(Amarok::lastfmApiKey()) );
        query.addQueryItem( QStringLiteral("album"), sanitizeQuery( album()->name() ) );

        if( album()->hasAlbumArtist() )
        {
            query.addQueryItem( QStringLiteral("artist"), sanitizeQuery( album()->albumArtist()->name() ) );
        }
        query.addQueryItem( QStringLiteral("method"), method() );
        url.setQuery( query );

        metadata[ QStringLiteral("source") ] = QStringLiteral("Last.fm");
        metadata[ QStringLiteral("method") ] = method();
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
        if( xml.isStartElement() && xml.name() == QStringLiteral("searchresults") )
        {
            while( !xml.atEnd() && !xml.hasError() )
            {
                xml.readNext();
                const QStringView &n = xml.name();
                if( xml.isEndElement() && n == QStringLiteral("searchresults") )
                    break;
                if( !xml.isStartElement() )
                    continue;
                if( n == QStringLiteral("result") )
                {
                    while( !xml.atEnd() && !xml.hasError() )
                    {
                        xml.readNext();
                        if( xml.isEndElement() && n == QStringLiteral("result") )
                            break;
                        if( !xml.isStartElement() )
                            continue;
                        if( xml.name() == QStringLiteral("uri") )
                        {
                            QUrl releaseUrl( xml.readElementText() );
                            QString releaseStr = releaseUrl.adjusted(QUrl::StripTrailingSlash).toString();
                            QString releaseId = releaseStr.split( QLatin1Char('/') ).last();

                            QUrl url;
                            url.setScheme( QStringLiteral("https") );
                            url.setHost( QStringLiteral("www.discogs.com") );
                            url.setPath( QStringLiteral("/release/") + releaseId );
                            QUrlQuery query;
                            query.addQueryItem( QStringLiteral("api_key"), QLatin1String(Amarok::discogsApiKey()) );
                            query.addQueryItem( QStringLiteral("f"), QStringLiteral("xml") );
                            url.setQuery( query );

                            CoverFetch::Metadata metadata;
                            metadata[ QStringLiteral("source") ] = QStringLiteral("Discogs");

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
    url.setScheme( QStringLiteral("https") );
    CoverFetch::Metadata metadata;

    switch( m_src )
    {
    default:
    case CoverFetch::LastFm:
        url.setHost( QStringLiteral("ws.audioscrobbler.com") );
        url.setPath( QStringLiteral("/2.0/") );
        query.addQueryItem( QStringLiteral("api_key"), QLatin1String(Amarok::lastfmApiKey()) );
        query.addQueryItem( QStringLiteral("limit"), QString::number( 20 ) );
        query.addQueryItem( QStringLiteral("page"), QString::number( m_page ) );
        query.addQueryItem( QStringLiteral("album"), sanitizeQuery( m_query ) );
        query.addQueryItem( QStringLiteral("method"), method() );
        metadata[ QStringLiteral("source") ] = QStringLiteral("Last.fm");
        metadata[ QStringLiteral("method") ] = method();
        break;

    case CoverFetch::Discogs:
        debug() << "Setting up a Discogs fetch";
        url.setHost( QStringLiteral("www.discogs.com") );
        url.setPath( QStringLiteral("/search") );
        query.addQueryItem( QStringLiteral("api_key"), QLatin1String(Amarok::discogsApiKey()) );
        query.addQueryItem( QStringLiteral("page"), QString::number( m_page + 1 ) );
        query.addQueryItem( QStringLiteral("type"), QStringLiteral("all") );
        query.addQueryItem( QStringLiteral("q"), sanitizeQuery( m_query ) );
        query.addQueryItem( QStringLiteral("f"), QStringLiteral("xml") );
        debug() << "Discogs Url: " << url;
        metadata[ QStringLiteral("source") ] = QStringLiteral("Discogs");
        break;

    case CoverFetch::Google:
        url.setHost( QStringLiteral("images.google.com") );
        url.setPath( QStringLiteral("/images") );
        query.addQueryItem( QStringLiteral("q"), sanitizeQuery( m_query ) );
        query.addQueryItem( QStringLiteral("gbv"), QStringLiteral( "1" ) );
        query.addQueryItem( QStringLiteral("filter"), QStringLiteral( "1" ) );
        query.addQueryItem( QStringLiteral("start"), QString::number( 20 * m_page ) );
        metadata[ QStringLiteral("source") ] = QStringLiteral("Google");
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
        if( !xml.isStartElement() || xml.name() != QStringLiteral("release") )
            continue;

        const QString releaseId = xml.attributes().value( "id" ).toString();
        while( !xml.atEnd() && !xml.hasError() )
        {
            xml.readNext();
            const QStringView &n = xml.name();
            if( xml.isEndElement() && n == QStringLiteral("release") )
                break;
            if( !xml.isStartElement() )
                continue;

            CoverFetch::Metadata metadata;
            metadata[ QStringLiteral("source") ] = "Discogs";
            if( n == QStringLiteral("title") )
                metadata[ QStringLiteral("title") ] = xml.readElementText();
            else if( n == QStringLiteral("country") )
                metadata[ QStringLiteral("country") ] = xml.readElementText();
            else if( n == QStringLiteral("released") )
                metadata[ QStringLiteral("released") ] = xml.readElementText();
            else if( n == QStringLiteral("notes") )
                metadata[ QStringLiteral("notes") ] = xml.readElementText();
            else if( n == QStringLiteral("images") )
            {
                while( !xml.atEnd() && !xml.hasError() )
                {
                    xml.readNext();
                    if( xml.isEndElement() && xml.name() == QStringLiteral("images") )
                        break;
                    if( !xml.isStartElement() )
                        continue;
                    if( xml.name() == QStringLiteral("image") )
                    {
                        const QXmlStreamAttributes &attr = xml.attributes();
                        const QUrl thburl( attr.value( "uri150" ).toString() );
                        const QUrl uri( attr.value( "uri" ).toString() );
                        const QUrl url = (m_size == CoverFetch::ThumbSize) ? thburl : uri;
                        if( !url.isValid() )
                            continue;

                        metadata[ QStringLiteral("releaseid")    ] = releaseId;
                        metadata[ QStringLiteral("releaseurl")   ] = QStringLiteral("http://discogs.com/release/") + releaseId;
                        metadata[ QStringLiteral("normalarturl") ] = uri.url();
                        metadata[ QStringLiteral("thumbarturl")  ] = thburl.url();
                        metadata[ QStringLiteral("width")        ] = attr.value( "width"  ).toString();
                        metadata[ QStringLiteral("height")       ] = attr.value( "height" ).toString();
                        metadata[ QStringLiteral("type")         ] = attr.value( "type"   ).toString();
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
    if( method() == QStringLiteral("album.getinfo") )
    {
        artistSet << normalize( ( album() && album()->albumArtist() )
                                ? album()->albumArtist()->name()
                                : i18n( "Unknown Artist" ) );
    }
    else if( method() == QStringLiteral("album.search") )
    {
        if( !m_wild && album() )
        {
            const Meta::TrackList tracks = album()->tracks();
            QStringList artistNames( QStringLiteral("Various Artists") );
            for( const Meta::TrackPtr &track : tracks )
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
        if( !xml.isStartElement() || xml.name() != QStringLiteral("album") )
            continue;

        QHash<QString, QString> coverUrlHash;
        CoverFetch::Metadata metadata;
        metadata[ QStringLiteral("source") ] = QStringLiteral("Last.fm");
        while( !xml.atEnd() && !xml.hasError() )
        {
            xml.readNext();
            const QStringView &n = xml.name();
            if( xml.isEndElement() && n == QStringLiteral("album") )
                break;
            if( !xml.isStartElement() )
                continue;

            if( n == QStringLiteral("name") )
            {
                metadata[ QStringLiteral("name") ] = xml.readElementText();
            }
            else if( n == QStringLiteral("artist") )
            {
                const QString &artist = xml.readElementText();
                if( !artistSet.contains( artist ) )
                    continue;
                metadata[ QStringLiteral("artist") ] = artist;
            }
            else if( n == QStringLiteral("url") )
            {
                metadata[ QStringLiteral("releaseurl") ] = xml.readElementText();
            }
            else if( n == QStringLiteral("image") )
            {
                QString sizeStr = xml.attributes().value("size").toString();
                coverUrlHash[ sizeStr ] = xml.readElementText();
            }
        }

        QStringList acceptableSizes;
        acceptableSizes << QStringLiteral("large") << QStringLiteral("medium") << QStringLiteral("small");
        metadata[ QStringLiteral("thumbarturl") ] = firstAvailableValue( acceptableSizes, coverUrlHash );

        acceptableSizes.clear();
        acceptableSizes << QStringLiteral("extralarge") << QStringLiteral("large");
        metadata[ QStringLiteral("normalarturl") ] = firstAvailableValue( acceptableSizes, coverUrlHash );

        QUrl url( m_size == CoverFetch::ThumbSize ? metadata[QStringLiteral("thumbarturl")] : metadata[QStringLiteral("normalarturl")] );
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
    const QRegularExpression spaceRegExp  = QRegularExpression( QStringLiteral("\\s") );
    return raw.toLower().remove( spaceRegExp ).normalized( QString::NormalizationForm_KC );
}

QStringList
CoverFetchArtPayload::normalize( const QStringList &rawList )
{
    QStringList cooked;
    for( const QString &raw : rawList )
    {
        cooked << normalize( raw );
    }
    return cooked;
}

