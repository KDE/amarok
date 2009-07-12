/****************************************************************************************
 * Copyright (c) 2005,2006,2009 Martin Aumueller <aumuell@reserv.at>                    *
 * Copyright (c) 2004 Christian Muehlhaeuser <chris@chris.de>                           *
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * query method from Amarok 1.4.10                                                      *
 * Copyright (c) 2004 Mark Kretschmann <markey@web.de>                                  *
 * Copyright (c) 2004 Christian Muehlhaeuser <chris@chris.de>                           *
 * Copyright (c) 2004 Sami Nieminen <sami.nieminen@iki.fi>                              *
 * Copyright (c) 2005 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2005 Jeff Mitchell <kde-dev@emailgoeshere.com>                         *
 * Copyright (c) 2005 Isaiah Damron <xepo@trifault.net>                                 *
 * Copyright (c) 2005-2006 Alexandre Pereira de Oliveira <aleprj@gmail.com>             *
 * Copyright (c) 2006 Jonas Hurrelmann <j@outpo.st>                                     *
 * Copyright (c) 2006 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>                             *
 * Copyright (c) 2006 Stanislav Nikolov <valsinats@gmail.com>                           *
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

#define DEBUG_PREFIX "IphoneOsHandler"

#include "IphoneOsHandler.h"

#include "IphoneOsCollection.h"
#include "capabilities/IphoneOsReadCapability.h"
#include "Debug.h"

#include <KCodecs> // KMD5
#include <KIO/Job>
#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/Scheduler>
#include <KIO/NetAccess>
#include "kjob.h"
#include <threadweaver/ThreadWeaver.h>
#include <KUrl>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutexLocker>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QTime>
#include <QProcess>

#include <sqlite3.h>

using namespace Meta;

IphoneOsHandler::IphoneOsHandler( IphoneOsCollection *mc, const QString &mountpoint )
    : MediaDeviceHandler( mc )
    , m_mountPoint( mountpoint )
    , m_currentMeta( new MediaDeviceTrack( mc ) )
{
    DEBUG_BLOCK

    m_copyingthreadsafe = false;

    m_success = false;

}

IphoneOsHandler::~IphoneOsHandler()
{
    DEBUG_BLOCK

    sqlite3_close(m_libraryDb);
    sqlite3_close(m_dynamicDb);
    sqlite3_close(m_locationsDb);

    int result = QProcess::execute("fusermount -u " + mountPoint());
    debug() << "unmounting" << (result ? "failed" : "worked");
}

/// Capability-related functions

bool
IphoneOsHandler::hasCapabilityInterface( Handler::Capability::Type type ) const
{
    switch( type )
    {
        case Handler::Capability::Readable:
            return true;

        default:
            return false;
    }
}

Handler::Capability*
IphoneOsHandler::createCapabilityInterface( Handler::Capability::Type type )
{
    switch( type )
    {
        case Handler::Capability::Readable:
            return new Handler::IphoneOsReadCapability( this );

        default:
            return 0;
    }
}

void
IphoneOsHandler::init()
{
    DEBUG_BLOCK

    int result = QProcess::execute(QString("mount.fuse.ifuse iphone " + mountPoint()));
    debug() << "mounting" << (result ? "failed" : "worked");

    QString librarydb = mountPoint() + "/iTunes_Control/iTunes/iTunes Library.itlp/Library.itdb";
    debug() << "trying to sqlite3_open " << librarydb;
    result = sqlite3_open_v2(librarydb.toLocal8Bit().data(), &m_libraryDb, SQLITE_OPEN_READONLY, NULL);
    debug() << "opened library db: " << result;

    QString dynamicdb = mountPoint() + "/iTunes_Control/iTunes/iTunes Library.itlp/Dynamic.itdb";
    result = sqlite3_open_v2(dynamicdb.toLocal8Bit().data(), &m_dynamicDb, SQLITE_OPEN_READONLY, NULL);
    debug() << "opened dynamic db: " << result;

    QString locationsdb = mountPoint() + "/iTunes_Control/iTunes/iTunes Library.itlp/Locations.itdb";
    result = sqlite3_open_v2(locationsdb.toLocal8Bit().data(), &m_locationsDb, SQLITE_OPEN_READONLY, NULL);
    debug() << "opened locations db: " << result;

    m_memColl->slotAttemptConnectionDone( result==SQLITE_OK );
}

QString
IphoneOsHandler::prettyName() const
{
    return QString::fromUtf8( "iPhone" );
}

MediaDeviceTrackPtr &
IphoneOsHandler::metaForTrack( const MediaDeviceTrackPtr &track)
{
    QString pid = m_trackhash[track];
    return m_currentMeta;
}

MediaDeviceTrackPtr &
IphoneOsHandler::metaForPid(const QString &pid)
{
    if(pid != m_currentPid)
        return m_currentMeta;

    MediaDeviceTrackPtr &meta = m_currentMeta;

    QString sql = QString(
            "SELECT "
            "item.title, item.artist, item.album, item.composer, item.genre_id, item.year, item.track_number, item.disc_number, "
            "item.total_time_ms, item.comment, item.description, item.description_long, "
            "avformat_info.bit_rate, avformat_info.sample_rate "
            "FROM item "
            "INNER JOIN avformat_info ON item.pid=avformat_info.item_pid "
            //"INNER JOIN genre_map ON item.genre_id=genre_map.id "
            "WHERE item.pid='%1';"
            ).arg(pid);
    QStringList libResult = query(m_libraryDb, sql);
    if(libResult.size() < 14)
    {
        debug() << "track lib" << m_currentPidIndex << m_currentPid << libResult;
        while(libResult.size() < 20)
            libResult.push_back(QString("ERROR!!!"));
    }
    meta->setTitle(libResult[0]);
    meta->setArtist(libResult[1]);
    meta->setAlbum(libResult[2]);
    meta->setComposer(libResult[3]);
    if(libResult[4] == "0")
    {
        meta->setGenre(QString());
    }
    else
    {
        QStringList genreResult = query(m_libraryDb, QString("SELECT genre FROM genre_map WHERE id='%1';").arg(libResult[4]));
        if(genreResult.size() >= 1)
            meta->setGenre(genreResult[0]);
        else
            meta->setGenre(QString());
    }
    meta->setYear(libResult[5]);
    meta->setTrackNumber(libResult[6].toInt());
    meta->setDiscNumber(libResult[7].toInt());
    meta->setLength(libResult[8].toDouble()/1000.+.5); // round
    QString comment = libResult[9];
    if(comment.isEmpty())
        comment = libResult[10];
    if(comment.isEmpty())
        comment = libResult[11];
    meta->setBitrate(libResult[12].toDouble());
    meta->setSamplerate(libResult[13].toDouble());

    sql = QString(
            "SELECT "
            "item_stats.user_rating, item_stats.date_played, item_stats.play_count_recent "
            "FROM item_stats "
            "WHERE item_stats.item_pid='%1';"
            ).arg(pid);
    QStringList dynResult = query(m_dynamicDb, sql);
    if(dynResult.size() < 3)
    {
        debug() << "track dyn" << m_currentPidIndex << dynResult;
        while(dynResult.size() < 10)
            dynResult.push_back(QString("ERROR!!!"));
    }
    meta->setRating(dynResult[0].toInt()/10);
    meta->setLastPlayed(dynResult[1].toInt());
    meta->setPlayCount(dynResult[2].toInt());

    sql = QString(
            "SELECT "
            "base_location.path, location.location, location.file_size, location.date_created, location.extension "
            "FROM location "
            "INNER JOIN base_location ON location.base_location_id=base_location.id "
            "WHERE location.item_pid='%1';"
            ).arg(pid);
    QStringList locResult = query(m_locationsDb, sql);
    if(locResult.size() < 5)
    {
        debug() << "track loc" << m_currentPidIndex << locResult;
        while(locResult.size() < 10)
            locResult.push_back(QString("ERROR!!!"));
    }
    meta->setPlayableUrl(mountPoint() + "/" + locResult[0] + "/" + locResult[1]);
    meta->setFileSize(locResult[2].toInt());
    uint extId = locResult[4].toUInt();
    char extString[5] = { (extId>>24)&0xff, (extId>>16)&0xff, (extId>>8)&0xff, extId&0xff, '\0' };
    QString type = QString::fromUtf8(extString);
    meta->setType(type.simplified().toLower());

    return m_currentMeta;
}

/* copied from Amarok 1.4.10, collectiondb.cpp */
QStringList
IphoneOsHandler::query(sqlite3 *db, const QString &statement)
{
    QStringList values;

    int error;
    int rc = 0;
    const char* tail;
    sqlite3_stmt* stmt;
    int busyCnt = 0;
    int retryCnt = 0;

    do {
        //compile SQL program to virtual machine, reattempting if busy
        do {
            if ( busyCnt )
            {
                ::usleep( 100000 );      // Sleep 100 msec
                debug() << "sqlite3_prepare: BUSY counter: " << busyCnt << endl;
            }
            error = sqlite3_prepare( db, statement.toUtf8().data(), -1, &stmt, &tail );
        }
        while ( SQLITE_BUSY==error && busyCnt++ < 120 );

        if ( error != SQLITE_OK )
        {
            if ( SQLITE_BUSY==error )
                debug() << "Gave up waiting for lock to clear" << endl;
            debug() << k_funcinfo << " sqlite3_compile error:" << endl;
            debug() << sqlite3_errmsg( db ) << endl;
            debug() << "on query: " << statement << endl;
            values = QStringList();
            break;
        }
        else
        {
            busyCnt = 0;
            int number = sqlite3_column_count( stmt );
            //execute virtual machine by iterating over rows
            while(true)
            {
                error = sqlite3_step( stmt );

                if ( error == SQLITE_BUSY )
                {
                    if ( busyCnt++ > 120 ) {
                        debug() << "Busy-counter has reached maximum. Aborting this sql statement!\n";
                        break;
                    }
                    ::usleep( 100000 ); // Sleep 100 msec
                    debug() << "sqlite3_step: BUSY counter: " << busyCnt << endl;
                    continue;
                }
                if ( error == SQLITE_MISUSE )
                    debug() << "sqlite3_step: MISUSE" << endl;
                if ( error == SQLITE_ERROR )
                {
                    debug() << "sqlite3_step: ERROR";
                    break;
                }
                if ( error == SQLITE_DONE )
                    break;

                if ( error != SQLITE_ROW )
                    debug() << "sqlite3_step: weird error";

                //iterate over columns
                for ( int i = 0; i < number; i++ )
                {
                    values << QString::fromUtf8( reinterpret_cast<const char*>( sqlite3_column_text( stmt, i ) ) );
                }
            }
            //deallocate vm resources
            rc = sqlite3_finalize( stmt );

            if ( error != SQLITE_DONE && rc != SQLITE_SCHEMA )
            {
                debug() << k_funcinfo << "sqlite_step error.\n";
                debug() << sqlite3_errmsg( db ) << endl;
                debug() << "on query: " << statement << endl;
                values = QStringList();
            }
            if ( rc == SQLITE_SCHEMA )
            {
                retryCnt++;
                debug() << "SQLITE_SCHEMA error occurred on query: " << statement << endl;
                if ( retryCnt < 10 )
                    debug() << "Retrying now." << endl;
                else
                {
                    debug() << "Retry-Count has reached maximum. Aborting this SQL statement!" << endl;
                    debug() << "SQL statement: " << statement << endl;
                    values = QStringList();
                }
            }
        }
    }
    while ( rc == SQLITE_SCHEMA && retryCnt < 10 );

    return values;
}

void
IphoneOsHandler::prepareToParseTracks()
{
    QString statement = "SELECT pid FROM item WHERE is_song='1';";
    m_pids = query(m_libraryDb, statement);
    m_currentPidIndex = 0;

    prepareToParseNextTrack();
}

bool
IphoneOsHandler::isEndOfParseTracksList()
{
    return m_currentPidIndex >= m_pids.size()-1;
}

void
IphoneOsHandler::prepareToParseNextTrack()
{
    m_currentPid = m_pids[m_currentPidIndex];
    metaForPid(m_currentPid);
}

void
IphoneOsHandler::nextTrackToParse()
{
    ++m_currentPidIndex;
}

void
IphoneOsHandler::setAssociateTrack( const Meta::MediaDeviceTrackPtr track )
{
    m_trackhash[ track ] = m_pids[m_currentPidIndex];
}

#include "IphoneOsHandler.moc"
