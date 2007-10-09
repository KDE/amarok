/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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
#include "sqlcollection.h"

#include <QFileInfo>
#include <QHash>
#include <QListIterator>
#include <QStringList>
#include <QXmlStreamAttributes>

#include <threadweaver/ThreadWeaver.h>

ScanManager::ScanManager( SqlCollection *parent )
    :QObject( parent )
    , m_collection( parent )
    , m_scanner( 0 )
    , m_parser( 0 )
    , m_restartCount( 0 )
{
    //nothing to do
}

void
ScanManager::startFullScan()
{
}

void ScanManager::startIncrementalScan()
{
    m_scanner = new KProcess( this );
    *m_scanner << "amarokcollectionscanner" << "--nocrashhandler" << "--i";
    if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
    *m_scanner << getDirsToScan();
    m_scanner->start();
    if( m_parser )
    {
        //TODO remove old parser, make sure this code actually works
        m_parser->requestAbort();
        ThreadWeaver::Weaver::instance()->dequeue( m_parser );
        m_parser->deleteLater();
    }
    m_parser = new XmlParseJob( this );
    ThreadWeaver::Weaver::instance()->enqueue( m_parser );
}

void
ScanManager::slotReadReady()
{
    DEBUG_BLOCK
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
ScanManager::slotFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    DEBUG_BLOCK
    if( exitStatus == QProcess::CrashExit )
    {
        //TODO handle collection scanner crash
        m_restartCount++;
    }
    else
    {
        //make sure that we read the complete buffer
        slotReadReady();
        m_scanner->deleteLater();
        m_scanner = 0;
        m_restartCount = 0;
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
            QString( "SELECT deviceid, dir, changedate FROM directories WHERE deviceid IN (%1);" )
            .arg( deviceIds ) );

    QStringList result;
    for( QListIterator<QString> iter( values ); iter.hasNext(); )
    {
        int id = iter.next().toInt();
        const QString folder = MountPointManager::instance()->getAbsolutePath( id, iter.next() );
        const uint mtime = iter.next().toUInt();

        QFileInfo info( folder );
        if( info.exists() )
        {
            if( info.lastModified().toTime_t() != mtime )
            {
                result << folder;
                debug() << "Collection dir changed: " << folder;
            }
        }
        else
        {
            // this folder has been removed
            result << folder;
            debug() << "Collection dir removed: " << folder;
        }
    }
    return result;
}


//XmlParseJob

XmlParseJob::XmlParseJob( ScanManager *parent )
    : ThreadWeaver::Job( parent )
{
}

XmlParseJob::~XmlParseJob()
{
    //nothing to do
}

void
XmlParseJob::run()
{
    QMap<QString, QHash<QString, QString> > audioFileData;
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
                    audioFileData.insert( attrs.value( "url" ).toString(), data );
                }
                else if( localname == "folder" )
                {
                    QXmlStreamAttributes attrs = m_reader.attributes();
                    const QString folder = attrs.value( "path" ).toString();
                    const QFileInfo info( folder );

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
