/****************************************************************************************
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

#include "XesamCollectionBuilder.h"

#include "Debug.h"
#include "core/meta/support/MetaUtility.h"
#include "SqlCollection.h"
#include "XesamDbus.h"

#include <QDBusConnection>
#include <QDBusReply>
#include <QDir>
#include <QXmlStreamWriter>

#include <kurl.h>

static const QString XESAM_NS = "http://freedesktop.org/standards/xesam/1.0/query";

#define DEBUG_XML true

XesamCollectionBuilder::XesamCollectionBuilder( SqlCollection *collection )
    : QObject( collection )
    , m_collection( collection )
{
    DEBUG_BLOCK
    m_xesam = new OrgFreedesktopXesamSearchInterface( "org.freedesktop.xesam.searcher",
                                                  "/org/freedesktop/xesam/searcher/main",
                                                  QDBusConnection::sessionBus() );
    if( m_xesam->isValid() )
    {
        connect( m_xesam, SIGNAL( HitsAdded( QString , int ) ), SLOT( slotHitsAdded( QString, int ) ) );
        connect( m_xesam, SIGNAL( HitsModified( QString, QList<int> ) ), SLOT( slotHitsModified( QString, QList<int> ) ) );
        connect( m_xesam, SIGNAL( HitsRemoved( QString, QList<int> ) ), SLOT( slotHitsRemoved( QString, QList<int> ) ) );
        QDBusReply<QString> sessionId = m_xesam->NewSession();
        if( !sessionId.isValid() )
        {
            debug() << "Could not acquire Xesam session, aborting. error was: " << sessionId.error();
            return;
            //TODO error handling
        }
        m_session = sessionId.value();
        if( !setupXesam() )
        {
            debug() << "Warning, could not setup xesam correctly";
        }
        QDBusReply<QString> search = m_xesam->NewSearch( m_session, generateXesamQuery() );
        if( search.isValid() )
        {
            m_search = search.value();
            m_xesam->StartSearch( m_search );
        }
        else
        {
            debug() << "Invalid response for NewSearch";
        }
    }
    else
    {
        //TODO display warning about unavailable xesam daemon
    }

}

XesamCollectionBuilder::~XesamCollectionBuilder()
{
    if( m_xesam && m_xesam->isValid() )
        m_xesam->CloseSession( m_session );
}

bool
XesamCollectionBuilder::setupXesam()
{
    bool status = true;
    if( !m_xesam->SetProperty( m_session, "search.live", QDBusVariant( true ) ).value().variant().toBool() )
    {
        debug() << "could not select xesam live search mode";
        status = false;
    }
    QStringList fields;
    fields << Meta::Field::URL << Meta::Field::TITLE << Meta::Field::ALBUM << Meta::Field::ARTIST << Meta::Field::GENRE;
    fields << Meta::Field::COMPOSER << Meta::Field::YEAR << Meta::Field::COMMENT << Meta::Field::CODEC;
    fields << Meta::Field::BITRATE << Meta::Field::BPM << Meta::Field::TRACKNUMBER << Meta::Field::DISCNUMBER;
    fields << Meta::Field::FILESIZE << Meta::Field::LENGTH << Meta::Field::SAMPLERATE;
    fields << Meta::Field::ALBUMGAIN << Meta::Field::ALBUMPEAKGAIN;
    fields << Meta::Field::TRACKGAIN << Meta::Field::TRACKPEAKGAIN;
    m_xesam->SetProperty( m_session, "hit.fields", QDBusVariant( fields ) );
    QStringList fieldsExtended;
    m_xesam->SetProperty( m_session, "hit.fields.extended", QDBusVariant( fieldsExtended ) );
    m_xesam->SetProperty( m_session, "sort.primary", QDBusVariant( Meta::Field::URL ) );
    m_xesam->SetProperty( m_session, "search.blocking", QDBusVariant( false ) );
    return status;
}

void
XesamCollectionBuilder::slotHitsAdded( const QString &search, int count )
{
    DEBUG_BLOCK
    if( m_search != search )
        return;
    debug() << "New Xesam hits: " << count;
    QDBusReply<VariantListVector> reply = m_xesam->GetHits( m_search, count );
    if( reply.isValid() )
    {
        VariantListVector result = reply.value();
        if( result.isEmpty() )
            return;
        KUrl firstUrl( result[0][0].toString() );
        QString dir = firstUrl.directory();
        QList<QList<QVariant> > dirData;
        //rows are sorted by directory/uri
        foreach( QList<QVariant> row, result )
        {
            KUrl url( row[0].toString() );
            if( url.directory() == dir )
            {
                dirData.append( row );
            }
            else
            {
                processDirectory( dirData );
                dirData.clear();
                dir = url.directory();
            }
        }
    }
}

void
XesamCollectionBuilder::slotHitsModified( const QString &search, const QList<int> &hit_ids )
{
    DEBUG_BLOCK
    Q_UNUSED( hit_ids );
    if( m_search != search )
        return;
}

void
XesamCollectionBuilder::slotHitsRemoved( const QString &search, const QList<int> &hit_ids )
{
    DEBUG_BLOCK
    Q_UNUSED( hit_ids );
    if( m_search != search )
        return;
}

void
XesamCollectionBuilder::searchDone( const QString &search )
{
    DEBUG_BLOCK
    if( m_search != search )
        return;
}

QString
XesamCollectionBuilder::generateXesamQuery() const
{
    QStringList collectionFolders = m_collection->mountPointManager()->collectionFolders();
    QString result;
    QXmlStreamWriter writer( &result );
    writer.setAutoFormatting( DEBUG_XML );
    writer.writeStartElement( XESAM_NS, "request" );
    writer.writeStartElement( XESAM_NS, "query" );
    writer.writeAttribute( XESAM_NS, "content", "xesam:Audio" );
    if( collectionFolders.size() <= 1 )
    {
        QString folder = collectionFolders.isEmpty() ? QDir::homePath() : collectionFolders[0];
        writer.writeStartElement( XESAM_NS, "startsWith" );
        writer.writeTextElement( XESAM_NS, "field", "dc:uri" );
        writer.writeTextElement( XESAM_NS, "string", folder );
        writer.writeEndElement();
    }
    else
    {
        writer.writeStartElement( XESAM_NS, "or" );
        foreach( const QString &folder, collectionFolders )
        {
            writer.writeStartElement( XESAM_NS, "startsWith" );
            writer.writeTextElement( XESAM_NS, "field", "dc:uri" );
            writer.writeTextElement( XESAM_NS, "string", folder );
            writer.writeEndElement();
        }
        writer.writeEndElement();
    }
    writer.writeEndDocument();
    if( DEBUG_XML )
        debug() << result;
    return result;
}

void
XesamCollectionBuilder::processDirectory( const QList<QList<QVariant> > &data )
{
    //URL TITLE ALBUM ARTIST GENRE COMPOSER YEAR COMMENT CODEC BITRATE BPM TRACKNUMBER DISCNUMBER FILESIZE LENGTH SAMPLERATE ALBUMGAIN ALBUMPEAK TRACKGAIN TRACKPEAK

    //using the following heuristics:
    //if more than one album is in the dir, use the artist of each track as albumartist
    //if more than 60 files are in the dir, use the artist of each track as albumartist
    //if all tracks have the same artist, use it as albumartist
    //try to find the albumartist A: tracks must have the artist A or A feat. B (and variants)
    //if no albumartist could be found, it's a compilation
    QSet<QString> artists;
    QString album;
    bool multipleAlbums = false;
    if( !data.isEmpty() )
        album = data[0][2].toString();
    foreach(QList<QVariant> row, data )
    {
        artists.insert( row[3].toString() );
        if( row[2].toString() != album )
            multipleAlbums = true;
    }
    if( multipleAlbums || data.count() > 60 || artists.size() == 1 )
    {
        foreach(QList<QVariant> row, data )
        {
            int artist = artistId( row[3].toString() );
            addTrack( row, artist );
        }
    }
    else
    {
    }
}

void
XesamCollectionBuilder::addTrack( const QList<QVariant> &trackData, int albumArtistId )
{
    // from setupXesam():
    // 0: URL          6: YEAR            12: DISCNUMBER   18: TRACKGAIN
    // 1: TITLE        7: COMMENT         13: FILESIZE     19: TRACKPEAKGAIN
    // 2: ALBUM        8: CODEC           14: LENGTH
    // 3: ARTIST       9: BITRATE         15: SAMPLERATE
    // 4: GENRE       10: BPM             16: ALBUMGAIN
    // 5: COMPOSER    11: TRACKNUMBER     17: ALBUMPEAKGAIN
    int album = albumId( trackData[2].toString(), albumArtistId );
    int artist = artistId( trackData[3].toString() );
    int genre = genreId( trackData[4].toString() );
    int composer = composerId( trackData[5].toString() );
    int year = yearId( trackData[6].toString() );
    int url = urlId( trackData[0].toString() );

    QString insert = "INSERT INTO tracks(url,artist,album,genre,composer,year,title,comment,"
                     "tracknumber,discnumber,bitrate,length,samplerate,filesize,filetype,bpm"
                     "createdate,modifydate,albumgain,albumpeakgain,trackgain,trackpeakgain) "
                     "VALUES ( %1,%2,%3,%4,%5,%6,'%7','%8'"; //goes up to comment
    insert = insert.arg( url ).arg( artist ).arg( album ).arg( genre ).arg( composer ).arg( year );
    insert = insert.arg( m_collection->sqlStorage()->escape( trackData[1].toString() ), m_collection->sqlStorage()->escape( trackData[7].toString() ) );

    QString insert2 = ",%1,%2,%3,%4,%5,%6,%7,%8,%9,%10,%11,%12,%13,%14);";
    // tracknumber, discnumber, bitrate
    insert2 = insert2.arg( trackData[11].toInt() ).arg( trackData[12].toInt() ).arg( trackData[9].toInt() );
    // length, samplerate, filesize
    insert2 = insert2.arg( trackData[14].toInt() ).arg( trackData[15].toInt() ).arg( trackData[13].toInt() );
    // filetype, bpm, createdate, modifydate
    insert2 = insert2.arg( "0", "0", "0", "0" ); //filetype, bpm, createdate, modifydate not implemented yet
    // FIXME: what format should we expect the *PeakGain values to be in?
    //        may need to do 20 * log10( received_value ) to get into decibels
    // albumgain
    insert2 = insert2.arg( trackData[16].toDouble() ).arg( trackData[17].toDouble() );
    // trackgain
    insert2 = insert2.arg( trackData[18].toDouble() ).arg( trackData[19].toDouble() );
    insert += insert2;

    m_collection->sqlStorage()->insert( insert, "tracks" );
}

int
XesamCollectionBuilder::artistId( const QString &artist )
{
    if( m_artists.contains( artist ) )
        return m_artists.value( artist );
    QString query = QString( "SELECT id FROM artists WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( artist ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO albums( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( artist ) );
        int id = m_collection->sqlStorage()->insert( insert, "albums" );
        m_artists.insert( artist, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_artists.insert( artist, id );
        return id;
    }
}

int
XesamCollectionBuilder::genreId( const QString &genre )
{
    if( m_genre.contains( genre ) )
        return m_genre.value( genre );
    QString query = QString( "SELECT id FROM genres WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( genre ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO genres( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( genre ) );
        int id = m_collection->sqlStorage()->insert( insert, "genre" );
        m_genre.insert( genre, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_genre.insert( genre, id );
        return id;
    }
}

int
XesamCollectionBuilder::composerId( const QString &composer )
{
    if( m_composer.contains( composer ) )
        return m_composer.value( composer );
    QString query = QString( "SELECT id FROM composers WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( composer ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO composers( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( composer ) );
        int id = m_collection->sqlStorage()->insert( insert, "composers" );
        m_composer.insert( composer, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_composer.insert( composer, id );
        return id;
    }
}

int
XesamCollectionBuilder::yearId( const QString &year )
{
    if( m_year.contains( year ) )
        return m_year.value( year );
    QString query = QString( "SELECT id FROM years WHERE name = '%1';" ).arg( m_collection->sqlStorage()->escape( year ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO years( name ) VALUES ('%1');" ).arg( m_collection->sqlStorage()->escape( year ) );
        int id = m_collection->sqlStorage()->insert( insert, "years" );
        m_year.insert( year, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_year.insert( year, id );
        return id;
    }
}

int 
XesamCollectionBuilder::albumId( const QString &album, int artistId )
{
    QPair<QString, int> key( album, artistId );
    if( m_albums.contains( key ) )
        return m_albums.value( key );

    QString query = QString( "SELECT id FROM albums WHERE artist = %1 AND name = '%2';" )
                        .arg( QString::number( artistId ), m_collection->sqlStorage()->escape( album ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO albums(artist, name) VALUES( %1, '%2' );" )
                    .arg( QString::number( artistId ), m_collection->sqlStorage()->escape( album ) );
        int id = m_collection->sqlStorage()->insert( insert, "albums" );
        m_albums.insert( key, id );
        return id;
    }
    else
    {
        int id = res[0].toInt();
        m_albums.insert( key, id );
        return id;
    }
}

int
XesamCollectionBuilder::urlId( const QString &url )
{
    int deviceId = m_collection->mountPointManager()->getIdForUrl( url );
    QString rpath = m_collection->mountPointManager()->getRelativePath( deviceId, url );
    //don't bother caching the data, we only call this method for each url once
    QString query = QString( "SELECT id FROM urls WHERE deviceid = %1 AND rpath = '%2';" )
                        .arg( QString::number( deviceId ), m_collection->sqlStorage()->escape( rpath ) );
    QStringList res = m_collection->sqlStorage()->query( query );
    if( res.isEmpty() )
    {
        QString insert = QString( "INSERT INTO urls(deviceid, rpath) VALUES ( %1, '%2' );" )
                            .arg( QString::number( deviceId ), m_collection->sqlStorage()->escape( rpath ) );
        return m_collection->sqlStorage()->insert( insert, "urls" );
    }
    else
    {
        return res[0].toInt();
    }
}

#include "XesamCollectionBuilder.moc"

