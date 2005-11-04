/***************************************************************************
 *   Copyright (C) 2003-2005 by The amaroK Developers                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "ScanController"

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "debug.h"
#include "metabundle.h"
#include "scancontroller.h"
#include "statusbar.h"

#include <qfileinfo.h>

#include <klocale.h>
#include <kprocio.h>

using amaroK::StatusBar;


////////////////////////////////////////////////////////////////////////////////
// class ScannerProcIO
////////////////////////////////////////////////////////////////////////////////
/** Due to xine-lib, we have to make KProcess close all fds, otherwise we get "device is busy" messages
  * Used by AmaroKProcIO and AmaroKProcess, exploiting commSetupDoneC(), a virtual method that
  * happens to be called in the forked process
  * See bug #103750 for more information.
  */
class ScannerProcIO : public KProcIO {
    public:
    virtual int commSetupDoneC() {
        const int i = KProcIO::commSetupDoneC();
        amaroK::closeOpenFiles( KProcIO::out[0],KProcIO::in[0],KProcIO::err[0] );
        return i;
    };
};


////////////////////////////////////////////////////////////////////////////////
// class ScanController
////////////////////////////////////////////////////////////////////////////////

ScanController* ScanController::s_instance = 0;


ScanController::ScanController( QObject* parent, bool incremental, const QStringList& folders )
    : QObject( parent )
    , QXmlDefaultHandler()
    , m_db( CollectionDB::instance()->getStaticDbConnection() )
    , m_scanner( new ScannerProcIO() )
    , m_folders( folders )
    , m_incremental( incremental )
    , m_steps( 0 )
{
    DEBUG_BLOCK

    s_instance = this;

    if( !m_db->isConnected() || ( m_incremental && !initIncrementalScanner() ) ) {
        deleteLater();
        return;
    }

    CollectionDB::instance()->createTables( m_db );

    StatusBar::instance()->newProgressOperation( this )
            .setDescription( m_incremental ? i18n( "Updating Collection" ) : i18n( "Building Collection" ) )
            .setAbortSlot( this, SLOT( deleteLater() ) )
            .setTotalSteps( 100 );

    m_reader.setContentHandler( this );
    m_reader.parse( &m_source, true );

    *m_scanner << "amarokcollectionscanner";
    if( AmarokConfig::importPlaylists() && !m_incremental ) *m_scanner << "-i";
    if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
    *m_scanner << m_folders;

    connect( m_scanner, SIGNAL( readReady( KProcIO* ) ),      SLOT( slotReadReady() ) );
    connect( m_scanner, SIGNAL( processExited( KProcess* ) ), SLOT( slotProcessExited() ) );

    m_scanner->start();
}


ScanController::~ScanController()
{
    DEBUG_BLOCK

    m_scanner->kill();
    delete m_scanner;

    if( m_db->isConnected() && !m_folders.isEmpty() ) {
        CollectionDB::instance()->dropTables( m_db );
        CollectionDB::instance()->returnStaticDbConnection( m_db );
    }

    emit CollectionDB::instance()->scanDone( !m_folders.isEmpty() );

    s_instance = 0;
}


bool
ScanController::initIncrementalScanner()
{
    /**
     * The Incremental Reader works as follows: Here we check the mtime of every directory in the "directories"
     * table and store all changed directories in m_folders.
     *
     * These directories are then scanned in CollectionReader::doJob(), with m_recursively set according to the
     * user's preference, so the user can add directories or whole directory trees, too. Since we don't want to
     * rescan unchanged subdirectories, CollectionReader::readDir() checks if we are scanning recursively and
     * prevents that.
     */

    DEBUG_BLOCK

    const QStringList values = CollectionDB::instance()->query( "SELECT dir, changedate FROM directories;" );

    foreach( values ) {
        const QString folder = *it;
        const QString mtime  = *++it;

        const QFileInfo info( folder );
        if( info.exists() ) {
            if( info.lastModified().toTime_t() != mtime.toUInt() ) {
                m_folders << folder;
                debug() << "Collection dir changed: " << folder << endl;
            }
        }
        else {
            // this folder has been removed
            m_folders << folder;
            debug() << "Collection dir removed: " << folder << endl;
        }
    }

    if( m_folders.isEmpty() )
        return false;

    StatusBar::instance()->shortMessage( i18n( "Updating Collection..." ) );
    return true;
}


bool
ScanController::startElement( const QString&, const QString& localName, const QString&, const QXmlAttributes& attrs )
{
    if( localName == "itemcount") {
        m_totalSteps = attrs.value( "count" ).toInt();
        debug() << "itemcount event: " << m_totalSteps << endl;
    }

    if( localName == "dud" || localName == "tags" ) { // Dud means invalid item
        m_steps++;
        const int newPercent = int( (100 * m_steps) / m_totalSteps);
        StatusBar::instance()->setProgress( this, newPercent );
    }

    if( localName == "tags") {
        MetaBundle bundle;

        bundle.setPath   ( attrs.value( "path" ) );
        bundle.setTitle  ( attrs.value( "title" ) );
        bundle.setArtist ( attrs.value( "artist" ) );
        bundle.setAlbum  ( attrs.value( "album" ) );
        bundle.setComment( attrs.value( "comment" ) );
        bundle.setGenre  ( attrs.value( "genre" ) );
        bundle.setYear   ( attrs.value( "year" ).toInt() );
        bundle.setTrack  ( attrs.value( "track" ).toInt() );

        if( attrs.value( "audioproperties" ) == "true" ) {
            bundle.setBitrate   ( attrs.value( "bitrate" ).toInt() );
            bundle.setLength    ( attrs.value( "length" ).toInt() );
            bundle.setSampleRate( attrs.value( "samplerate" ).toInt() );
        }

        CollectionDB::instance()->addSong( &bundle, m_incremental, m_db );
    }

    if( localName == "folder" ) {
        const QString folder = attrs.value( "path" );
        const QFileInfo info( folder );

        // Update dir statistics for rescanning purposes
        if( info.exists() )
            CollectionDB::instance()->updateDirStats( folder, info.lastModified().toTime_t(), !m_incremental ? m_db : 0 );
        else {
            if( m_incremental ) {
                CollectionDB::instance()->removeSongsInDir( folder );
                CollectionDB::instance()->removeDirFromCollection( folder );
            }
        }
    }

    return true;
}


void
ScanController::slotReadReady()
{
    QString line, data;
    bool partial;

    while( m_scanner->readln( line, true, &partial ) != -1 )
        data += line;

    m_source.setData( data );

    if( !m_reader.parseContinue() )
        ::warning() << "parseContinue() failed: " << errorString() << endl << data << endl;
}


void
ScanController::slotProcessExited()
{
    DEBUG_BLOCK

    if( m_scanner->normalExit() ) {
        if ( m_incremental )
            foreach( m_folders ) CollectionDB::instance()->removeSongsInDir( *it );
        else
            CollectionDB::instance()->clearTables();

        CollectionDB::instance()->moveTempTables( m_db ); // rename tables
    }
    else {
        ::error() << "CollectionScanner has crashed! Scan aborted." << endl;
        m_folders.clear();
    }

    deleteLater();
}


#include "scancontroller.moc"
