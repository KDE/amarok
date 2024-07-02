/****************************************************************************************
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#define DEBUG_PREFIX "DaapReader"

#include "Reader.h"

#include "authentication/contentfetcher.h"
#include "../DaapCollection.h"
#include "../DaapMeta.h"
#include "core/support/Debug.h"


#include <QByteArray>
#include <QDateTime>
#include <QDataStream>
#include <QVariant>

#include <ThreadWeaver/ThreadWeaver>
#include <ThreadWeaver/Queue>

using namespace Daap;
using namespace Meta;

//#define DEBUGTAG( VAR ) debug() << tag << " has value " << VAR;
#define DEBUGTAG( VAR )

Reader::Reader( Collections::DaapCollection* mc, const QString& host, quint16 port, const QString& password, QObject* parent, const char* name)
    : QObject( parent )
    , m_memColl( mc )
    , m_host( host )
    , m_port( port )
    , m_sessionId( -1 )
    , m_password( password )
{
    setObjectName( QLatin1String(name) );
    debug() << "Host: " << host << " port: " << port;

    // these content codes are needed to learn all others
    m_codes[QStringLiteral("mccr")] = Code( QStringLiteral("dmap.contentcodesresponse"), CONTAINER );
    m_codes[QStringLiteral("mstt")] = Code( QStringLiteral("dmap.status"), LONG );
    m_codes[QStringLiteral("mdcl")] = Code( QStringLiteral("dmap.dictionary"), CONTAINER );
    // mcnm is actually an int, but string makes parsing easier
    m_codes[QStringLiteral("mcnm")] = Code( QStringLiteral("dmap.contentcodesnumber"), STRING );
    m_codes[QStringLiteral("mcna")] = Code( QStringLiteral("dmap.contentcodesname"), STRING );
    m_codes[QStringLiteral("mcty")] = Code( QStringLiteral("dmap.contentcodestype"), SHORT );

    // stupid, stupid. The reflection just isn't good enough
    // to connect to an iPhoto server.
    m_codes[QStringLiteral("ppro")] = Code( QStringLiteral("dpap.protocolversion"), LONG );
    m_codes[QStringLiteral("avdb")] = Code( QStringLiteral("daap.serverdatabases"), CONTAINER );
    m_codes[QStringLiteral("adbs")] = Code( QStringLiteral("daap.databasesongs"), CONTAINER );
    m_codes[QStringLiteral("pret")] = Code( QStringLiteral("dpap.unknown"), CONTAINER );
}

Reader::~Reader()
{  }

void
Reader::logoutRequest()
{
    DEBUG_BLOCK
    ContentFetcher* http = new ContentFetcher( m_host, m_port, m_password, this, "readerLogoutHttp" );
    connect( http, &ContentFetcher::httpError, this, &Reader::fetchingError );
    connect( http, &ContentFetcher::finished, this, &Reader::logoutRequestFinished );
    http->getDaap( QStringLiteral("/logout?") + m_loginString );
}

void
Reader::logoutRequestFinished()
{
    DEBUG_BLOCK
    sender()->deleteLater();
    deleteLater();
}

void
Reader::loginRequest()
{
    DEBUG_BLOCK
    ContentFetcher* http = new ContentFetcher( m_host, m_port, m_password, this, "readerHttp");
    connect( http, &ContentFetcher::httpError, this, &Reader::fetchingError );
    connect( http, &ContentFetcher::finished, this, &Reader::contentCodesReceived );
    http->getDaap( QStringLiteral("/content-codes") );
}

void
Reader::contentCodesReceived()
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, &ContentFetcher::finished, this, &Reader::contentCodesReceived );

    QDataStream raw( http->results() );
    Map contentCodes = parse( raw );
    QList<QVariant> root = contentCodes[QStringLiteral("mccr")].toList();
    if( root.isEmpty() )
        return; //error
    root = root[0].toMap().value( QStringLiteral("mdcl") ).toList();
    for( const QVariant &v : root )
    {
        Map entry = v.toMap();
        QString code = entry.value( QStringLiteral("mcnm") ).toList().value( 0 ).toString();
        QString name = entry.value( QStringLiteral("mcna") ).toList().value( 0 ).toString();
        ContentTypes type = ContentTypes( entry.value( QStringLiteral("mcty") ).toList().value( 0 ).toInt() );
        if( !m_codes.contains( code ) && !code.isEmpty() && type > 0 )
        {
            m_codes[code] = Code( name, type );
            debug() << "Added DAAP code" << code << ":" << name << "with type" << type;
        }
    }

    connect( http, &ContentFetcher::loginRequired,
             this, &Reader::loginHeaderReceived );
    http->getDaap( QStringLiteral("/login") );
}

void
Reader::loginHeaderReceived()
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, &ContentFetcher::loginRequired,
                this, &Reader::loginHeaderReceived );

    Q_EMIT passwordRequired();
    http->deleteLater();

//     connect( http, &ContentFetcher::finished, this, &Reader::loginFinished );
}


void
Reader::loginFinished()
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, &ContentFetcher::finished, this, &Reader::loginFinished );

    QDataStream raw( http->results() );
    Map loginResults = parse( raw );
    QVariantList list = loginResults.value( QStringLiteral("mlog") ).toList();
    debug() << "list size is " << list.size();
    QVariantList innerList = list.value( 0 ).toMap().value( QStringLiteral("mlid") ).toList();
    debug() << "innerList size is " << innerList.size();
    if( innerList.isEmpty() )
    {
        http->deleteLater();
        return;
    }
    m_sessionId = innerList.value( 0 ).toInt();
    m_loginString = QStringLiteral("session-id=") + QString::number( m_sessionId );
    connect( http, &ContentFetcher::finished, this, &Reader::updateFinished );
    http->getDaap( QStringLiteral("/update?") + m_loginString );
}

void
Reader::updateFinished()
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, &ContentFetcher::finished, this, &Reader::updateFinished );

    QDataStream raw( http->results() );
    Map updateResults = parse( raw );
    if( updateResults[QLatin1String("mupd")].toList().isEmpty() )
        return; //error
    if( updateResults[QLatin1String("mupd")].toList()[0].toMap()[QStringLiteral("musr")].toList().isEmpty() )
        return; //error
    m_loginString = m_loginString + QStringLiteral("&revision-number=")  +
            QString::number( updateResults[QLatin1String("mupd")].toList()[0].toMap()[QLatin1String("musr")].toList()[0].toInt() );

    connect( http, &ContentFetcher::finished, this, &Reader::databaseIdFinished );
    http->getDaap( QStringLiteral("/databases?") + m_loginString );
}

void
Reader::databaseIdFinished()
{
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, &ContentFetcher::finished, this, &Reader::databaseIdFinished );

    QDataStream raw( http->results() );
    Map dbIdResults = parse( raw );
    m_databaseId = QString::number( dbIdResults[QStringLiteral("avdb")].toList()[0].toMap()[QStringLiteral("mlcl")].toList()[0].toMap()[QStringLiteral("mlit")].toList()[0].toMap()[QStringLiteral("miid")].toList()[0].toInt() );
    connect( http, &ContentFetcher::finished, this, &Reader::songListFinished );
    http->getDaap( QStringLiteral("/databases/%1/items?type=music&meta=dmap.itemid,dmap.itemname,daap.songformat,daap.songartist,daap.songalbum,daap.songtime,daap.songtracknumber,daap.songcomment,daap.songyear,daap.songgenre&%2")
                .arg( m_databaseId, m_loginString ) );

}

void
Reader::songListFinished()
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, &ContentFetcher::finished, this, &Reader::songListFinished );

    QByteArray result = http->results();
    http->deleteLater();

    ThreadWeaver::Queue::instance()->enqueue( QSharedPointer<ThreadWeaver::Job>(new WorkerThread( result, this, m_memColl )) );
}

bool
Reader::parseSongList( const QByteArray &data, bool set_collection )
{
    // The original implementation used parse(), which uses addElement() and
    // makes heavy usage of QMaps and QList which hurts performance very badly.
    // Therefore this function parses the daap responses directly into the
    // DaapCollection which is 27 times faster here and saves a slight bit of
    // heap space.
    // parse() and addElement() create a more qt like structure though and might be
    // kept for other daap tasks.

    DEBUG_BLOCK
    QDataStream raw( data );

    // Cache for music data
    QString itemId;
    QString format;
    QString title;
    QString artist;
    QString composer;
    QString comment;
    QString album;
    QString genre;
    int year = 0;
    qint32 trackNumber=0;
    qint32 songTime=0;

    while( !raw.atEnd() )
    {
        char rawTag[5];
        quint32 tagLength = getTagAndLength( raw, rawTag );

        if( tagLength == 0 )
            continue;

        QVariant tagData = readTagData( raw, rawTag, tagLength );

        if( !tagData.isValid() )
            continue;

        QString tag = QLatin1String( rawTag );

        if( m_codes[tag].type == CONTAINER )
        {
             parseSongList( tagData.toByteArray() );
             continue;
        }

        if( tag == QStringLiteral("astn") )
            trackNumber = tagData.toInt();
        else if( tag == QStringLiteral("asyr") )
            year = tagData.toInt();
        else if( tag == QStringLiteral("miid") )
            itemId = tagData.toString();
        else if(tag == QStringLiteral("astm") )
            songTime = tagData.toInt();
        else if( tag== QStringLiteral("asfm") )
            format = tagData.toString();
        else if( tag == QStringLiteral("minm") )
            title = tagData.toString();
        else if( tag == QStringLiteral("asal") )
            album = tagData.toString();
        else if( tag == QStringLiteral("asar") )
            artist = tagData.toString();
        else if( tag == QStringLiteral("ascp") )
            composer = tagData.toString();
        else if( tag == QStringLiteral("ascm") )
            comment = tagData.toString();
        else if( tag == QStringLiteral("asgn") )
            genre = tagData.toString();
    }

    if( !itemId.isEmpty() )
        addTrack( itemId, title, artist, composer, comment, album, genre, year, format, trackNumber, songTime );

    if( set_collection )
    {
        m_memColl->memoryCollection()->acquireWriteLock();
        m_memColl->memoryCollection()->setTrackMap( m_trackMap );
        m_memColl->memoryCollection()->setArtistMap( m_artistMap );
        m_memColl->memoryCollection()->setAlbumMap( m_albumMap );
        m_memColl->memoryCollection()->setGenreMap( m_genreMap );
        m_memColl->memoryCollection()->setComposerMap( m_composerMap );
        m_memColl->memoryCollection()->setYearMap( m_yearMap );
        m_memColl->memoryCollection()->releaseLock();
        m_trackMap.clear();
        m_artistMap.clear();
        m_albumMap.clear();
        m_genreMap.clear();
        m_composerMap.clear();
        m_yearMap.clear();
    }
    return true;
}

void
Reader::addTrack( const QString& itemId, const QString& title, const QString& artist, const QString& composer,
                  const QString& comment, const QString& album, const QString& genre, int year, const QString& format,
                  qint32 trackNumber, qint32 songTime )
{
    DaapTrackPtr track( new DaapTrack( m_memColl, m_host, m_port, m_databaseId, itemId, format ) );
    track->setTitle( title );
    track->setLength( songTime );
    track->setTrackNumber( trackNumber );
    track->setComment( comment );
    track->setComposer( composer );

    DaapArtistPtr artistPtr;
    if ( m_artistMap.contains( artist ) )
        artistPtr = DaapArtistPtr::staticCast( m_artistMap.value( artist ) );
    else
    {
        artistPtr = DaapArtistPtr( new DaapArtist( artist ) );
        m_artistMap.insert( artist, ArtistPtr::staticCast( artistPtr ) );
    }
    artistPtr->addTrack( track );
    track->setArtist( artistPtr );

    DaapAlbumPtr albumPtr;
    if ( m_albumMap.contains( album, artist ) )
        albumPtr = DaapAlbumPtr::staticCast( m_albumMap.value( album, artist ) );
    else
    {
        albumPtr = DaapAlbumPtr( new DaapAlbum( album ) );
        albumPtr->setAlbumArtist( artistPtr );
        m_albumMap.insert( AlbumPtr::staticCast( albumPtr ) );
    }
    albumPtr->addTrack( track );
    track->setAlbum( albumPtr );

    DaapComposerPtr composerPtr;
    if ( m_composerMap.contains( composer ) )
        composerPtr = DaapComposerPtr::staticCast( m_composerMap.value( composer ) );
    else
    {
        composerPtr = DaapComposerPtr( new DaapComposer ( composer ) );
        m_composerMap.insert( composer, ComposerPtr::staticCast( composerPtr ) );
    }
    composerPtr->addTrack( track );
    track->setComposer ( composerPtr );

    DaapYearPtr yearPtr;
    if ( m_yearMap.contains( year ) )
        yearPtr = DaapYearPtr::staticCast( m_yearMap.value( year ) );
    else
    {
        yearPtr = DaapYearPtr( new DaapYear( QString::number(year) ) );
        m_yearMap.insert( year, YearPtr::staticCast( yearPtr ) );
    }
    yearPtr->addTrack( track );
    track->setYear( yearPtr );

    DaapGenrePtr genrePtr;
    if ( m_genreMap.contains( genre ) )
        genrePtr = DaapGenrePtr::staticCast( m_genreMap.value( genre ) );
    else
    {
        genrePtr = DaapGenrePtr( new DaapGenre( genre ) );
        m_genreMap.insert( genre, GenrePtr::staticCast( genrePtr ) );
    }
    genrePtr->addTrack( track );
    track->setGenre( genrePtr );
    m_trackMap.insert( track->uidUrl(), TrackPtr::staticCast( track ) );
}


quint32
Reader::getTagAndLength( QDataStream &raw, char tag[5] )
{
    tag[4] = 0;
    raw.readRawData(tag, 4);
    quint32 tagLength = 0;
    raw >> tagLength;
    return tagLength;
}

QVariant
Reader::readTagData( QDataStream &raw, char *tag, quint32 tagLength)
{
    /**
    * Consume tagLength bytes of data from the stream and convert it to the
    * proper type, while making sure that datalength/datatype mismatches are handled properly
    */

    QVariant ret = QVariant();

    if ( tagLength == 0 )
        return ret;

#define READ_DATA(var) \
    DEBUGTAG( var ) \
    if( sizeof(var) != tagLength ) { \
        warning() << "Bad tag data length:" << tag << ":" << tagLength; \
        raw.skipRawData(tagLength); \
        break; \
    } else { \
        raw >> var ; \
        ret = QVariant(var); \
    }
    switch( m_codes[QLatin1String(tag)].type )
    {
        case CHAR:
        {
            qint8 charData;
            READ_DATA( charData )
            break;
        }
        case SHORT:
        {
            qint16 shortData;
            READ_DATA( shortData )
            break;
        }
        case LONG:
        {
            qint32 longData;
            READ_DATA( longData );
            break;
        }
        case LONGLONG:
        {
            qint64 longlongData;
            READ_DATA( longlongData );
            break;
        }
        case STRING:
        {
            QByteArray stringData( tagLength, ' ' );
            raw.readRawData( stringData.data(), tagLength );
            ret = QVariant(QString::fromUtf8( stringData, tagLength ));
            DEBUGTAG( QString::fromUtf8( stringData, tagLength ) )
            break;
        }
        case DATE:
        {
            qint64 dateData;
            READ_DATA( dateData )
            QDateTime date;
            date.setSecsSinceEpoch( dateData );
            ret = QVariant( date );
            break;
        }
        case DVERSION:
        {
            qint32 verData;
            READ_DATA( verData )
            QString version( QStringLiteral("%1.%2.%3") );
            version = version.arg( verData >> 16, (verData >> 8) & 0xFF, verData & 0xFF);
            ret = QVariant( version );
            break;
        }
        case CONTAINER:
        {
            QByteArray containerData( tagLength, ' ' );
            raw.readRawData( containerData.data(), tagLength );
            ret = QVariant( containerData );
            break;
        }
        default:
            warning() << "Tag" << tag << "has unhandled type.";
            raw.skipRawData(tagLength);
            break;
    }
#undef READ_DATA
    return ret;
}

Map
Reader::parse( QDataStream &raw )
{
    DEBUG_BLOCK
    /**
     * http://daap.sourceforge.net/docs/index.html
     * 0-3     Content code    OSType (unsigned long), description of the contents of this chunk
     * 4-7     Length  Length of the contents of this chunk (not the whole chunk)
     * 8-      Data    The data contained within the chunk
     **/
    Map childMap;
    while( !raw.atEnd() )
    {
        char tag[5];
        quint32 tagLength = getTagAndLength( raw, tag );
        if( tagLength == 0 )
            continue;

        QVariant tagData = readTagData(raw, tag, tagLength);
        if( !tagData.isValid() )
            continue;

        if( m_codes[QLatin1String(tag)].type == CONTAINER )
        {
            QDataStream substream( tagData.toByteArray() );
            addElement( childMap, tag, QVariant( parse( substream ) ) );
        }
        else
            addElement( childMap, tag, tagData );
    }
    return childMap;
}

void
Reader::addElement( Map &parentMap, char* tag, const QVariant &element )
{
    QList<QVariant> list;
    Map::Iterator it = parentMap.find( QLatin1String(tag) );
    if ( it == parentMap.end() ) {
        list.append( element );
        parentMap.insert( QLatin1String(tag), QVariant( list ) );
    } else {
        list = it.value().toList();
        list.append( element );
        it.value() = QVariant( list );
    }
}

void
Reader::fetchingError( const QString& error )
{
    DEBUG_BLOCK
    sender()->deleteLater();
    Q_EMIT httpError( error );
}

WorkerThread::WorkerThread( const QByteArray &data, Reader *reader, Collections::DaapCollection *coll )
    : QObject()
    , ThreadWeaver::Job()
    , m_success( false )
    , m_data( data )
    , m_reader( reader )
{
    connect( this, &WorkerThread::done, coll, &Collections::DaapCollection::loadedDataFromServer );
    connect( this, &WorkerThread::failed, coll, &Collections::DaapCollection::parsingFailed );
    connect( this, &WorkerThread::done, this, &Reader::deleteLater );
}

WorkerThread::~WorkerThread()
{
    //nothing to do
}

bool
WorkerThread::success() const
{
    return m_success;
}

void
WorkerThread::run(ThreadWeaver::JobPointer self, ThreadWeaver::Thread *thread)
{
    Q_UNUSED(self);
    Q_UNUSED(thread);
    m_success = m_reader->parseSongList( m_data, true );
}

void
WorkerThread::defaultBegin(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    Q_EMIT started(self);
    ThreadWeaver::Job::defaultBegin(self, thread);
}

void
WorkerThread::defaultEnd(const ThreadWeaver::JobPointer& self, ThreadWeaver::Thread *thread)
{
    ThreadWeaver::Job::defaultEnd(self, thread);
    if (!self->success()) {
        Q_EMIT failed(self);
    }
    Q_EMIT done(self);
}
