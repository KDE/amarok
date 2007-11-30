/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *  Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>
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

#include "ScanManager.h"

#include "amarokconfig.h"
#include "debug.h"
#include "meta/MetaConstants.h"
#include "meta/MetaUtility.h"
#include "mountpointmanager.h"
#include "ScanResultProcessor.h"
#include "SqlCollection.h"

#include <QFileInfo>
#include <QHash>
#include <QListIterator>
#include <QStringList>
#include <QXmlStreamAttributes>

#include <threadweaver/ThreadWeaver.h>

static const int MAX_RESTARTS = 80;
static const int MAX_FAILURE_PERCENTAGE = 5;

ScanManager::ScanManager( SqlCollection *parent )
    :QObject( parent )
    , m_collection( parent )
    , m_scanner( 0 )
    , m_parser( 0 )
    , m_restartCount( 0 )
    , m_isIncremental( false )
{
    //nothing to do
}

void
ScanManager::startFullScan()
{
    DEBUG_BLOCK
    if( m_parser )
    {
        debug() << "scanner already running";
        return;
    }
    m_scanner = new Process( this );
    *m_scanner << "amarokcollectionscanner" << "--nocrashhandler";
    if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
    *m_scanner << MountPointManager::instance()->collectionFolders();
    m_scanner->setOutputChannelMode( KProcess::OnlyStdoutChannel );
    connect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
    connect( m_scanner, SIGNAL( finished( int ) ), SLOT( slotFinished(  ) ) );
    connect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotError( QProcess::ProcessError ) ) );
    m_scanner->start();
    if( m_parser )
    {
        //TODO remove old parser, make sure this code actually works
        m_parser->requestAbort();
        ThreadWeaver::Weaver::instance()->dequeue( m_parser );
        m_parser->deleteLater();
    }
    m_parser = new XmlParseJob( this, m_collection );
    m_parser->setIsIncremental( false );
    m_isIncremental = false;
    connect( m_parser, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( slotJobDone() ) );
    ThreadWeaver::Weaver::instance()->enqueue( m_parser );
}

void ScanManager::startIncrementalScan()
{
    DEBUG_BLOCK
    if( m_parser )
    {
        debug() << "scanner already running";
        return;
    }
    m_scanner = new Process( this );
    *m_scanner << "amarokcollectionscanner" << "--nocrashhandler" << "--i";
    if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
    *m_scanner << getDirsToScan();
    m_scanner->setOutputChannelMode( KProcess::OnlyStdoutChannel );
    connect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
    connect( m_scanner, SIGNAL( finished( int ) ), SLOT( slotFinished(  ) ) );
    connect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotError( QProcess::ProcessError ) ) );
    m_scanner->start();
    if( m_parser )
    {
        //TODO remove old parser, make sure this code actually works
        m_parser->requestAbort();
        ThreadWeaver::Weaver::instance()->dequeue( m_parser );
        m_parser->deleteLater();
    }
    m_parser = new XmlParseJob( this, m_collection );
    m_parser->setIsIncremental( true );
    m_isIncremental = true;
    connect( m_parser, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( slotJobDone() ) );
    ThreadWeaver::Weaver::instance()->enqueue( m_parser );
}

bool
ScanManager::isDirInCollection( QString path )
{
    if ( path.endsWith( "/" ) )
        path = path.left( path.length() - 1 );
    int deviceid = MountPointManager::instance()->getIdForUrl( path );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, path );

    QStringList values =
            m_collection->query( QString( "SELECT changedate FROM directories WHERE dir = '%2' AND deviceid = %1;" )
            .arg( deviceid )
            .arg( m_collection->escape( rpath ) ) );

    return !values.isEmpty();
}

bool
ScanManager::isFileInCollection( const QString &url  )
{
    int deviceid = MountPointManager::instance()->getIdForUrl( url );
    QString rpath = MountPointManager::instance()->getRelativePath( deviceid, url );

    QString sql = QString( "SELECT id FROM urls WHERE rpath = '%2' AND deviceid = %1" )
            .arg( deviceid )
            .arg( m_collection->escape( rpath ) );
    if ( deviceid == -1 )
    {
        sql += ';';
    }
    else
    {
        QString rpath2 = '.' + url;
        sql += QString( " OR rpath = '%1' AND deviceid = -1;" )
                .arg( m_collection->escape( rpath2 ) );
    }
    QStringList values = m_collection->query( sql );

    return !values.isEmpty();
}

void
ScanManager::slotReadReady()
{
    QByteArray line;
    QString newData;
    line = m_scanner->readLine();

    while( !line.isEmpty() ) {
        //important! see
        //http://www.qtcentre.org/forum/f-general-programming-9/t-passing-to-a-console-application-managed-via-qprocess-utf-8-encoded-parameters-5375.html
        //for an explanation of the QString::fromLocal8Bit call
        QString data = QString::fromLocal8Bit( line );
        if( !data.startsWith( "exepath=" ) ) // skip binary location info from scanner
            newData += data;
        line = m_scanner->readLine();
    }
    if( m_parser )
        m_parser->addNewXmlData( newData );
}

void
ScanManager::slotFinished( )
{
    DEBUG_BLOCK

    //make sure that we read the complete buffer
    slotReadReady();
    m_scanner->deleteLater();
    m_scanner = 0;
    m_restartCount = 0;
}

void
ScanManager::slotError( QProcess::ProcessError error )
{
    if( error == QProcess::Crashed )
    {
        handleRestart();
    }
}

QStringList
ScanManager::getDirsToScan() const
{
    IdList list = MountPointManager::instance()->getMountedDeviceIds();
    QString deviceIds;
    foreach( int id, list )
    {
        if ( !deviceIds.isEmpty() ) deviceIds += ',';
        deviceIds += QString::number( id );
    }

    const QStringList values = m_collection->query(
            QString( "SELECT id, deviceid, dir, changedate FROM directories WHERE deviceid IN (%1);" )
            .arg( deviceIds ) );

    QList<int> changedFolderIds;

    QStringList result;
    for( QListIterator<QString> iter( values ); iter.hasNext(); )
    {
        int id = iter.next().toInt();
        int deviceid = iter.next().toInt();
        const QString folder = MountPointManager::instance()->getAbsolutePath( deviceid, iter.next() );
        const uint mtime = iter.next().toUInt();

        QFileInfo info( folder );
        if( info.exists() )
        {
            if( info.lastModified().toTime_t() != mtime )
            {
                result << folder;
                changedFolderIds << id;
                debug() << "Collection dir changed: " << folder;
            }
        }
        else
        {
            // this folder has been removed
            result << folder;
            changedFolderIds << id;
            debug() << "Collection dir removed: " << folder;
        }
    }
    {
        QString ids;
        foreach( int id, changedFolderIds )
        {
            if( !ids.isEmpty() )
                ids += ',';
            ids += id;
        }
        QString query = QString( "SELECT id FROM urls WHERE directory IN ( %1 );" ).arg( ids );
        QStringList urlIds = m_collection->query( query );
        ids.clear();
        foreach( const QString &id, urlIds )
        {
            if( !ids.isEmpty() )
                ids += ',';
            ids += id;
        }
        QString sql = QString( "DELETE FROM tracks WHERE url IN ( %1 );" ).arg( ids );
        m_collection->query( sql );
    }
    return result;
}

void
ScanManager::slotJobDone()
{
    m_parser->deleteLater();
    m_parser = 0;
}

void
ScanManager::handleRestart()
{
    //TODO handle collection scanner crash
    m_restartCount++;
    if( m_restartCount >= MAX_RESTARTS )
    {
        //TODO:abort scan, inform user
    }
    else
    {
        if( m_parser )
        {
        //TODO remove old parser, make sure this code actually works
            m_parser->requestAbort();
            ThreadWeaver::Weaver::instance()->dequeue( m_parser );
            m_parser->deleteLater();
            m_parser = 0;
        }
        delete m_scanner;
        m_scanner = new Process( this );
        *m_scanner << "amarokcollectionscanner" << "--nocrashhandler";
        if( m_isIncremental )
        {
            *m_scanner << "-i";
        }
        *m_scanner << "-s";
        m_scanner->setOutputChannelMode( KProcess::OnlyStdoutChannel );
        connect( m_scanner, SIGNAL( readyReadStandardOutput() ), this, SLOT( slotReadReady() ) );
        connect( m_scanner, SIGNAL( finished( int ) ), SLOT( slotFinished(  ) ) );
        connect( m_scanner, SIGNAL( error( QProcess::ProcessError ) ), SLOT( slotError( QProcess::ProcessError ) ) );
        m_scanner->start();
        m_parser = new XmlParseJob( this, m_collection );
        m_parser->setIsIncremental( m_isIncremental );
        connect( m_parser, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( slotJobDone() ) );
        ThreadWeaver::Weaver::instance()->enqueue( m_parser );
    }
}

//XmlParseJob

XmlParseJob::XmlParseJob( ScanManager *parent, SqlCollection *collection )
    : ThreadWeaver::Job( parent )
    , m_collection( collection )
    , m_isIncremental( false )
{
}

XmlParseJob::~XmlParseJob()
{
    //nothing to do
}

void
XmlParseJob::setIsIncremental( bool incremental )
{
    m_isIncremental = incremental;
}

void
XmlParseJob::run()
{
    DEBUG_BLOCK
    QMap<QString, QHash<QString, QString> > audioFileData;
    QMap<QString, uint> directories;
    do
    {
        //get new xml data or wait till new xml data is available
        m_mutex.lock();
        if( m_nextData.isEmpty() )
        {
            m_wait.wait( &m_mutex );
        }
        m_reader.addData( m_nextData );
        m_nextData.clear();
        m_mutex.unlock();

        while( !m_reader.atEnd() )
        {
            m_reader.readNext();
            if( m_reader.isStartElement() )
            {
                QStringRef localname = m_reader.name();
                if( localname == "dud" || localname == "tags" || localname == "playlist" )
                {
                    //TODO increment progress
                }
                if( localname == "itemcount" )
                {
                    //TODO handle itemcount
                }
                else if( localname == "tags" )
                {
                    //TODO handle tag data
                    QXmlStreamAttributes attrs = m_reader.attributes();
                    QHash<QString, QString> data;
                    data.insert( Meta::Field::URL, attrs.value( "path" ).toString() );
                    data.insert( Meta::Field::TITLE, attrs.value( "title" ).toString() );
                    data.insert( Meta::Field::ARTIST, attrs.value( "artist" ).toString() );
                    data.insert( Meta::Field::COMPOSER, attrs.value( "composer" ).toString() );
                    data.insert( Meta::Field::ALBUM, attrs.value( "album" ).toString() );
                    data.insert( Meta::Field::COMMENT, attrs.value( "comment" ).toString() );
                    data.insert( Meta::Field::GENRE, attrs.value( "genre" ).toString() );
                    data.insert( Meta::Field::YEAR, attrs.value( "year" ).toString() );
                    data.insert( Meta::Field::TRACKNUMBER, attrs.value( "track" ).toString() );
                    data.insert( Meta::Field::DISCNUMBER, attrs.value( "discnumber" ).toString() );
                    data.insert( Meta::Field::BPM, attrs.value( "bpm" ).toString() );
                    //filetype and uniqueid are missing in the fields, compilation is not used here
                    if( attrs.value( "audioproperties" ) == "true" )
                    {
                        data.insert( Meta::Field::BITRATE, attrs.value( "bitrate" ).toString() );
                        data.insert( Meta::Field::LENGTH, attrs.value( "length" ).toString() );
                        data.insert( Meta::Field::SAMPLERATE, attrs.value( "samplerate" ).toString() );
                    }
                    if( !attrs.value( "filesize" ).isEmpty() )
                        data.insert( Meta::Field::FILESIZE, attrs.value( "filesize" ).toString() );
                    audioFileData.insert( attrs.value( "path" ).toString(), data );
                }
                else if( localname == "folder" )
                {
                    QXmlStreamAttributes attrs = m_reader.attributes();
                    const QString folder = attrs.value( "path" ).toString();
                    const QFileInfo info( folder );
                    directories.insert( folder, info.lastModified().toTime_t() );

                    /*// Update dir statistics for rescanning purposes
                    if( info.exists() )
                        CollectionDB::instance()->updateDirStats( folder, info.lastModified().toTime_t(), true);

                    if( m_incremental ) {
                        m_foldersToRemove += folder;
                    }*/
                }
                else if( localname == "playlist" )
                {
                    //TODO handle playlist
                }
            }
        }
    }
    while( m_reader.error() == QXmlStreamReader::PrematureEndOfDocumentError );
    if( m_reader.error() != QXmlStreamReader::NoError )
    {
        //the error cannot be PrematureEndOfDocumentError, so handle
        //an unrecoverable error here
        //TODO implement
    }
    else
    {
        ScanResultProcessor processor( m_collection );
        if( m_isIncremental )
        {
            processor.setScanType( ScanResultProcessor::IncrementalScan );
        }
        else
        {
            processor.setScanType( ScanResultProcessor::FullScan );
        }
        foreach( const QString &dir, directories.keys() )
        {
            processor.addDirectory( dir, directories.value( dir ) );
        }
        processor.processScanResult( audioFileData );
    }
}

void
XmlParseJob::addNewXmlData( const QString &data )
{
    m_mutex.lock();
    //append the new xml data because the parser thread
    //might not have retrieved all xml data yet
    m_nextData += data;
    m_wait.wakeOne();
    m_mutex.unlock();
}

#include "ScanManager.moc"
