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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "DaapReader"

#include "Reader.h"

#include "authentication/contentfetcher.h"
#include "DaapCollection.h"
#include "DaapMeta.h"
#include "Debug.h"
#include "statusbar/StatusBar.h"

#include <QByteArray>
#include <QDateTime>
#include <QDataStream>
#include <QHttpResponseHeader>
#include <QVariant>

#include <threadweaver/ThreadWeaver.h>

using namespace Daap;
using namespace Meta;

QMap<QString, Code> Reader::s_codes;


Reader::Reader( DaapCollection* mc, const QString& host, quint16 port, const QString& password, QObject* parent, const char* name)
    : QObject( parent )
    , m_memColl( mc )
    , m_host( host )
    , m_port( port )
    , m_sessionId( -1 )
    , m_password( password )
    , m_loadedTracks( 0 )
{
    setObjectName( name );
    debug() << "Host: " << host << " port: " << port;

    if( s_codes.size() == 0 )
    {
        s_codes["mtco"] = Code( "dmap.specifiedtotalcount", LONG );
        s_codes["mdcl"] = Code( "dmap.dictionary", CONTAINER  );
        s_codes["aeGI"] = Code( "com.apple.itunes.itms-genreid", LONG );
        s_codes["aeNV"] = Code( "com.apple.itunes.norm-volume", LONG );
        s_codes["astn"] = Code( "daap.songtracknumber", SHORT );
        s_codes["abal"] = Code( "daap.browsealbumlisting", CONTAINER  );
        s_codes["asco"] = Code( "daap.songcompilation", CHAR );
        s_codes["aeSP"] = Code( "com.apple.itunes.smart-playlist", CHAR );
        s_codes["ascp"] = Code( "daap.songcomposer", STRING );
        s_codes["aseq"] = Code( "daap.songeqpreset", STRING );
        s_codes["abpl"] = Code( "daap.baseplaylist", CHAR );
        s_codes["msqy"] = Code( "dmap.supportsquery", CHAR );
        s_codes["aeCI"] = Code( "com.apple.itunes.itms-composerid", LONG );
        s_codes["mcnm"] = Code( "dmap.contentcodesnumber", LONG );
        s_codes["abro"] = Code( "daap.databasebrowse", CONTAINER  );
        s_codes["assz"] = Code( "daap.songsize", LONG );
        s_codes["abcp"] = Code( "daap.browsecomposerlisting", CONTAINER  );
        s_codes["aeAI"] = Code( "com.apple.itunes.itms-artistid", LONG );
        s_codes["aeHV"] = Code( "com.apple.itunes.has-video", CHAR );
        s_codes["msts"] = Code( "dmap.statusstring", STRING );
        s_codes["msas"] = Code( "dmap.authenticationschemes", LONG );
        s_codes["ascr"] = Code( "daap.songcontentrating", CHAR );
        s_codes["aePI"] = Code( "com.apple.itunes.itms-playlistid", LONG );
        s_codes["mstt"] = Code( "dmap.status", LONG );
        s_codes["msix"] = Code( "dmap.supportsindex", CHAR );
        s_codes["msrs"] = Code( "dmap.supportsresolve", CHAR );
        s_codes["mccr"] = Code( "dmap.contentcodesresponse", CONTAINER  );
        s_codes["asdk"] = Code( "daap.songdatakind", CHAR );
        s_codes["asar"] = Code( "daap.songartist", STRING );
        s_codes["ascs"] = Code( "daap.songcodecsubtype", LONG );
        s_codes["msau"] = Code( "dmap.authenticationmethod", CHAR );
        s_codes["aeSU"] = Code( "com.apple.itunes.season-num", LONG );
        s_codes["arif"] = Code( "daap.resolveinfo", CONTAINER  );
        s_codes["asct"] = Code( "daap.songcategory", STRING );
        s_codes["asfm"] = Code( "daap.songformat", STRING );
        s_codes["aeEN"] = Code( "com.apple.itunes.episode-num-str", STRING );
        s_codes["apsm"] = Code( "daap.playlistshufflemode", CHAR );
        s_codes["abar"] = Code( "daap.browseartistlisting", CONTAINER  );
        s_codes["mslr"] = Code( "dmap.loginrequired", CHAR );
        s_codes["msex"] = Code( "dmap.supportsextensions", CHAR );
        s_codes["mudl"] = Code( "dmap.deletedidlisting", CONTAINER  );
        s_codes["asdm"] = Code( "daap.songdatemodified", DATE );
        s_codes["asky"] = Code( "daap.songkeywords", STRING );
        s_codes["asul"] = Code( "daap.songdataurl", STRING );
        s_codes["aeSV"] = Code( "com.apple.itunes.music-sharing-version", LONG );
        s_codes["f\215ch"] = Code( "dmap.haschildcontainers", CHAR );
        s_codes["mlcl"] = Code( "dmap.listing", CONTAINER  );
        s_codes["msrv"] = Code( "dmap.serverinforesponse", CONTAINER  );
        s_codes["asdn"] = Code( "daap.songdiscnumber", SHORT );
        s_codes["astc"] = Code( "daap.songtrackcount", SHORT );
        s_codes["apso"] = Code( "daap.playlistsongs", CONTAINER  );
        s_codes["ascd"] = Code( "daap.songcodectype", LONG );
        s_codes["minm"] = Code( "dmap.itemname", STRING );
        s_codes["mimc"] = Code( "dmap.itemcount", LONG );
        s_codes["mctc"] = Code( "dmap.containercount", LONG );
        s_codes["aeSF"] = Code( "com.apple.itunes.itms-storefrontid", LONG );
        s_codes["asrv"] = Code( "daap.songrelativevolume", SHORT );
        s_codes["msup"] = Code( "dmap.supportsupdate", CHAR );
        s_codes["mcna"] = Code( "dmap.contentcodesname", STRING );
        s_codes["agrp"] = Code( "daap.songgrouping", STRING );
        s_codes["mikd"] = Code( "dmap.itemkind", CHAR );
        s_codes["mupd"] = Code( "dmap.updateresponse", CONTAINER  );
        s_codes["aeNN"] = Code( "com.apple.itunes.network-name", STRING );
        s_codes["asyr"] = Code( "daap.songyear", SHORT );
        s_codes["aeES"] = Code( "com.apple.itunes.episode-sort", LONG );
        s_codes["miid"] = Code( "dmap.itemid", LONG );
        s_codes["msbr"] = Code( "dmap.supportsbrowse", CHAR );
        s_codes["muty"] = Code( "dmap.updatetype", CHAR );
        s_codes["mcty"] = Code( "dmap.contentcodestype", SHORT );
        s_codes["aply"] = Code( "daap.databaseplaylists", CONTAINER  );
        s_codes["aePP"] = Code( "com.apple.itunes.is-podcast-playlist", CHAR );
        s_codes["aeSI"] = Code( "com.apple.itunes.itms-songid", LONG );
        s_codes["assp"] = Code( "daap.songstoptime", LONG );
        s_codes["aslc"] = Code( "daap.songlongcontentdescription", STRING );
        s_codes["mcon"] = Code( "dmap.container", CONTAINER  );
        s_codes["mlit"] = Code( "dmap.listingitem", CONTAINER  );
        s_codes["asur"] = Code( "daap.songuserrating", CHAR );
        s_codes["mspi"] = Code( "dmap.supportspersistentids", CHAR );
        s_codes["assr"] = Code( "daap.songsamplerate", LONG );
        s_codes["asda"] = Code( "daap.songdateadded", DATE );
        s_codes["asbr"] = Code( "daap.songbitrate", SHORT );
        s_codes["mcti"] = Code( "dmap.containeritemid", LONG );
        s_codes["mpco"] = Code( "dmap.parentcontainerid", LONG );
        s_codes["msdc"] = Code( "dmap.databasescount", LONG );
        s_codes["mlog"] = Code( "dmap.loginresponse", CONTAINER  );
        s_codes["mlid"] = Code( "dmap.sessionid", LONG );
        s_codes["musr"] = Code( "dmap.serverrevision", LONG );
        s_codes["asdb"] = Code( "daap.songdisabled", CHAR );
        s_codes["asdt"] = Code( "daap.songdescription", STRING );
        s_codes["mbcl"] = Code( "dmap.bag", CONTAINER  );
        s_codes["msal"] = Code( "dmap.supportsautologout", CHAR );
        s_codes["mstm"] = Code( "dmap.timeoutinterval", LONG );
        s_codes["asdc"] = Code( "daap.songdisccount", SHORT );
        s_codes["asbt"] = Code( "daap.songbeatsperminute", SHORT );
        s_codes["asgn"] = Code( "daap.songgenre", STRING );
        s_codes["aprm"] = Code( "daap.playlistrepeatmode", CHAR );
        s_codes["asst"] = Code( "daap.songstarttime", LONG );
        s_codes["mper"] = Code( "dmap.persistentid", LONGLONG );
        s_codes["mrco"] = Code( "dmap.returnedcount", LONG );
        s_codes["mpro"] = Code( "dmap.protocolversion", DVERSION );
        s_codes["ascm"] = Code( "daap.songcomment", STRING );
        s_codes["aePC"] = Code( "com.apple.itunes.is-podcast", CHAR );
        s_codes["aeSN"] = Code( "com.apple.itunes.series-name", STRING );
        s_codes["arsv"] = Code( "daap.resolve", CONTAINER  );
        s_codes["asal"] = Code( "daap.songalbum", STRING );
        s_codes["apro"] = Code( "daap.protocolversion", DVERSION );
        s_codes["avdb"] = Code( "daap.serverdatabases", CONTAINER  );
        s_codes["aeMK"] = Code( "com.apple.itunes.mediakind", CHAR );
        s_codes["astm"] = Code( "daap.songtime", LONG );
        s_codes["adbs"] = Code( "daap.databasesongs", CONTAINER  );
        s_codes["abgn"] = Code( "daap.browsegenrelisting", CONTAINER  );
        s_codes["ascn"] = Code( "daap.songcontentdescription", STRING );
    }
}

Reader::~Reader()
{  }

void
Reader::logoutRequest()
{
    DEBUG_BLOCK
    ContentFetcher* http = new ContentFetcher( m_host, m_port, m_password, this, "readerLogoutHttp" );
    connect( http, SIGNAL( httpError( const QString& ) ), this, SLOT( fetchingError( const QString& ) ) );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( logoutRequest( int, bool ) ) );
    http->getDaap( "/logout?" + m_loginString );
}

void
Reader::logoutRequest( int, bool )
{
    DEBUG_BLOCK
    const_cast<QObject*>(sender())->deleteLater();
    deleteLater();
}

void
Reader::loginRequest()
{
    DEBUG_BLOCK
    ContentFetcher* http = new ContentFetcher( m_host, m_port, m_password, this, "readerHttp");
    connect( http, SIGNAL( httpError( const QString& ) ), this, SLOT( fetchingError( const QString& ) ) );
    connect( http, SIGNAL(  responseHeaderReceived( const QHttpResponseHeader & ) )
            , this, SLOT( loginHeaderReceived( const QHttpResponseHeader & ) ) );
    http->getDaap( "/login" );
}

void
Reader::loginHeaderReceived( const QHttpResponseHeader & resp )
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, SIGNAL(  responseHeaderReceived( const QHttpResponseHeader & ) )
            , this, SLOT( loginHeaderReceived( const QHttpResponseHeader & ) ) );
    if( resp.statusCode() == 401 /*authorization required*/)
    {
        emit passwordRequired();
        http->deleteLater();
        return;
    }
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( loginFinished( int, bool ) ) );
}


void
Reader::loginFinished( int /* id */, bool error )
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( loginFinished( int, bool ) ) );
    if( error )
    {
        http->deleteLater();
        return;
    }
    QDataStream raw( http->results() );
    Map loginResults = parse( raw , 0 ,true );
    debug() << "list size is " << loginResults["mlog"].toList().size();
    if( loginResults["mlog"].toList().size() == 0 )
        return;
    QVariant tmp = loginResults["mlog"].toList()[0];
    Debug::stamp();
    m_sessionId = tmp.toMap()["mlid"].toList()[0].toInt();
    Debug::stamp();
    m_loginString = "session-id=" + QString::number( m_sessionId );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( updateFinished( int, bool ) ) );
    http->getDaap( "/update?" + m_loginString );
}

void
Reader::updateFinished( int /*id*/, bool error )
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( updateFinished( int, bool ) ) );
    if( error )
    {
        http->deleteLater();
        warning() << "what is going on here? " << http->error();
        return;
    }

    QDataStream raw( http->results() );
    Map updateResults = parse( raw, 0, true );
    if( updateResults["mupd"].toList().isEmpty() )
        return; //error
    if( updateResults["mupd"].toList()[0].toMap()["musr"].toList().isEmpty() )
        return; //error
    m_loginString = m_loginString + "&revision-number="  +
            QString::number( updateResults["mupd"].toList()[0].toMap()["musr"].toList()[0].toInt() );

    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( databaseIdFinished( int, bool ) ) );
    http->getDaap( "/databases?" + m_loginString );

}

void
Reader::databaseIdFinished( int /*id*/, bool error )
{
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( databaseIdFinished( int, bool ) ) );
    if( error )
    {
        http->deleteLater();
        return;
    }

    QDataStream raw( http->results() );
    Map dbIdResults = parse( raw, 0, true );
    m_databaseId = QString::number( dbIdResults["avdb"].toList()[0].toMap()["mlcl"].toList()[0].toMap()["mlit"].toList()[0].toMap()["miid"].toList()[0].toInt() );
    connect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( songListFinished( int, bool ) ) );
    http->getDaap( QString("/databases/%1/items?type=music&meta=dmap.itemid,dmap.itemname,daap.songformat,daap.songartist,daap.songalbum,daap.songtime,daap.songtracknumber,daap.songcomment,daap.songyear,daap.songgenre&%2")
                .arg( m_databaseId, m_loginString ) );

}

void
Reader::songListFinished( int /*id*/, bool error )
{
    DEBUG_BLOCK
    ContentFetcher* http = (ContentFetcher*) sender();
    disconnect( http, SIGNAL( requestFinished( int, bool ) ), this, SLOT( songListFinished( int, bool ) ) );
    if( error )
    {
        http->deleteLater();
        return;
    }
    QByteArray result = http->results();
    http->deleteLater();

    ThreadWeaver::Weaver::instance()->enqueue( new WorkerThread( result, this, m_memColl ) );
}

bool
Reader::parseSongList( const QByteArray &data )
{
    DEBUG_BLOCK
    QDataStream raw( data );

    // Cache for music data
    uint index = 0;
    QString itemId;
    QString format;
    QString title;
    QString artist;
    QString composer;
    QString comment;
    QString album;
    QString genre;
    QString year;
    qint32 trackNumber=0;
    qint32 songTime=0;
    bool isFirstTrack = false;
    bool first=true;
    uint containerLength=0;

    while( (first ? !raw.atEnd() : ( index < containerLength ) ) )
    {
        char tag[5];
        quint32 tagLength = getTagAndLength( raw, tag );
        if( tagLength == 0 )
        {
            index += 8;
            continue;
        }
//#define DEBUGTAG( VAR ) debug() << tag << " has value " << VAR;
#define DEBUGTAG( VAR )
        switch( s_codes[tag].type )
        {
            case CHAR: {
                qint8 charData;
                raw >> charData; DEBUGTAG( charData )
                }
                break;
            case SHORT: {
                qint16 shortData;
                raw >> shortData; DEBUGTAG( shortData )
                }
                break;
            case LONG: {
                qint32 longData;
                raw >> longData; DEBUGTAG( longData )
                if ( QString( tag ) == "miid" ) {
                    itemId = QString::number( longData );
                }
                if ( QString( tag ) == "astm" ) {
                    songTime = tagLength ? longData/1000 : 0;
                }
                if ( QString( tag ) == "astn" ) {
                    trackNumber = tagLength ? longData : 0;
                }
                }
                break;
            case LONGLONG: {
                qint64 longlongData;
                raw >> longlongData; DEBUGTAG( longlongData )
                }
                break;
            case STRING: {
                QByteArray stringData(tagLength, ' ');
                raw.readRawData( stringData.data(), tagLength ); DEBUGTAG( QString::fromUtf8( stringData, tagLength ) )
                if ( QString( tag ) == "asfm" ) {
                     format = tagLength ? QString::fromUtf8( stringData, tagLength ) : QString();
                }
                if ( QString( tag ) == "minm" ) {
                     title = tagLength ? QString::fromUtf8( stringData, tagLength ): QString();
                }
                if ( QString( tag ) == "asal" ) {
                     album = tagLength ? QString::fromUtf8( stringData, tagLength ): QString();
                }
                if ( QString( tag ) == "asar" ) {
                     artist = tagLength ? QString::fromUtf8( stringData, tagLength ): QString();
                }
                if ( QString( tag ) == "ascp" ) {
                     composer = tagLength ? QString::fromUtf8( stringData, tagLength ): QString();
                }
                if ( QString( tag ) == "ascm" ) {
                     comment = tagLength ? QString::fromUtf8( stringData, tagLength ): QString();
                }
                if ( QString( tag ) == "asyr" ) {
                     year = tagLength ? QString::fromUtf8( stringData, tagLength ): QString();
                }
                if ( QString( tag ) == "asgn" ) {
                     genre = tagLength ? QString::fromUtf8( stringData, tagLength ): QString();
                }
                }
                break;
            case DATE: {
                qint64 dateData;
                QDateTime date;
                raw >> dateData; DEBUGTAG( dateData )
                date.setTime_t(dateData);
                }
                break;
            case DVERSION: {
                qint16 major;
                qint8 minor;
                qint8 patchLevel;
                raw >> major >> minor >> patchLevel; DEBUGTAG( patchLevel )
                }
                break;
            case CONTAINER: {
                if ( !isFirstTrack && QString( tag ) == "mlit" ) {
                    addTrack( itemId, title, artist, composer, comment, album, genre, year, format, trackNumber, songTime );
                } else {
                    isFirstTrack=false;
                }

                DEBUGTAG( 11 )
                }
                break;
            default:
                warning() << tag << " does not work";
            break;
        }
        index += tagLength + 8;
    }


    if( isFirstTrack ) {
        emit httpError( "Invalid response" ); //it's not a real http error, but the effect is the same
        deleteLater();
        return false;
    } else { // add the last track which is otherwise lost
        addTrack( itemId, title, artist, composer, comment, album, genre, year, format, trackNumber, songTime );
    }

    m_memColl->acquireWriteLock();
    m_memColl->setTrackMap( m_trackMap );
    m_memColl->setArtistMap( m_artistMap );
    m_memColl->setAlbumMap( m_albumMap );
    m_memColl->setGenreMap( m_genreMap );
    m_memColl->setComposerMap( m_composerMap );
    m_memColl->setYearMap( m_yearMap );
    m_memColl->releaseLock();
    m_trackMap.clear();
    m_artistMap.clear();
    m_albumMap.clear();
    m_genreMap.clear();
    m_composerMap.clear();
    m_yearMap.clear();

    return true;
}

void
Reader::addTrack( const QString& itemId, const QString& title, const QString& artist, const QString& composer,
                  const QString& comment, const QString& album, const QString& genre, const QString& year, const QString& format,
                  qint32 trackNumber, qint32 songTime )
{
    DaapTrackPtr track( new DaapTrack( m_memColl, m_host, m_port, m_databaseId, itemId, format ) );
    track->setTitle( title );
    track->setLength( songTime );
    track->setTrackNumber( trackNumber );
    track->setComment( comment );
    track->setComment( composer );

    DaapAlbumPtr albumPtr;
    if ( m_albumMap.contains( album ) )
        albumPtr = DaapAlbumPtr::staticCast( m_albumMap.value( album ) );
    else
    {
        albumPtr = DaapAlbumPtr( new DaapAlbum( album ) );
        m_albumMap.insert( album, AlbumPtr::staticCast( albumPtr ) );
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

    DaapYearPtr yearPtr;
    if ( m_yearMap.contains( year ) )
        yearPtr = DaapYearPtr::staticCast( m_yearMap.value( year ) );
    else
    {
        yearPtr = DaapYearPtr( new DaapYear( year ) );
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

Map
Reader::parse( QDataStream &raw, uint containerLength, bool first )
{
DEBUG_BLOCK
/* http://daap.sourceforge.net/docs/index.html
0-3     Content code    OSType (unsigned long), description of the contents of this chunk
4-7     Length  Length of the contents of this chunk (not the whole chunk)
8-      Data    The data contained within the chunk */
    Map childMap;
    uint index = 0;
    while( (first ? !raw.atEnd() : ( index < containerLength ) ) )
    {
        char tag[5];
        quint32 tagLength = getTagAndLength( raw, tag );
        if( tagLength == 0 )
        {
            index += 8;
            continue;
        }
//#define DEBUGTAG( VAR ) debug() << tag << " has value " << VAR;
#define DEBUGTAG( VAR )
        switch( s_codes[tag].type )
        {
            case CHAR: {
                qint8 charData;
                raw >> charData; DEBUGTAG( charData )
                addElement( childMap, tag, QVariant( static_cast<int>(charData) ) );
                }
                break;
            case SHORT: {
                qint16 shortData;
                raw >> shortData; DEBUGTAG( shortData )
                addElement( childMap, tag, QVariant( static_cast<int>(shortData) ) );
                }
                break;
            case LONG: {
                qint32 longData;
                raw >> longData; DEBUGTAG( longData )
                addElement( childMap, tag, QVariant( longData ) );
                }
                break;
            case LONGLONG: {
                qint64 longlongData;
                raw >> longlongData; DEBUGTAG( longlongData )
                addElement( childMap, tag, QVariant( longlongData ) );
                }
                break;
            case STRING: {
                QByteArray stringData(tagLength, ' ');
                raw.readRawData( stringData.data(), tagLength ); DEBUGTAG( QString::fromUtf8( stringData, tagLength ) )
                addElement( childMap, tag, QVariant( QString::fromUtf8( stringData, tagLength ) ) );
                }
                break;
            case DATE: {
                qint64 dateData;
                QDateTime date;
                raw >> dateData; DEBUGTAG( dateData )
                date.setTime_t(dateData);
                addElement( childMap, tag, QVariant( date ) );
                }
                break;
            case DVERSION: {
                qint16 major;
                qint8 minor;
                qint8 patchLevel;
                raw >> major >> minor >> patchLevel; DEBUGTAG( patchLevel )
                QString version("%1.%2.%3");
                version.arg(major, minor, patchLevel);
                addElement( childMap, tag, QVariant(version) );
                }
                break;
            case CONTAINER: {
                DEBUGTAG( 11 )
                addElement( childMap, tag, QVariant( parse( raw, tagLength ) ) );
                }
                break;
            default:
                warning() << tag << " does not work";
            break;
        }
        index += tagLength + 8;
    }
    return childMap;
}

void
Reader::addElement( Map &parentMap, char* tag, QVariant element )
{
    QList<QVariant> list;
    Map::Iterator it = parentMap.find( tag );
    if ( it == parentMap.end() ) {
        list.append( element );
        parentMap.insert( tag, QVariant( list ) );
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
    const_cast< QObject* >( sender() )->deleteLater();
    emit httpError( error );
}

WorkerThread::WorkerThread( const QByteArray &data, Reader *reader, DaapCollection *coll )
    : ThreadWeaver::Job()
    , m_success( false )
    , m_data( data )
    , m_reader( reader )
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), coll, SLOT( loadedDataFromServer() ) );
    connect( this, SIGNAL( failed( ThreadWeaver::Job* ) ), coll, SLOT( parsingFailed() ) );
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), this, SLOT( deleteLater() ) );
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
WorkerThread::run()
{
    m_success = m_reader->parseSongList( m_data );
}

#include "Reader.moc"

