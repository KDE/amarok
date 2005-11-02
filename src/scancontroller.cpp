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

ScanController::ScanController( QObject* parent, QStringList folders )
    : QObject( parent )
    , QXmlDefaultHandler()
    , m_db( CollectionDB::instance()->getStaticDbConnection() )
    , m_scanner( new ScannerProcIO() )
    , m_steps( 0 )
    , m_success( false )
{
    DEBUG_BLOCK

    if( !m_db->isConnected() ) {
        deleteLater();
        return;
    }
    CollectionDB::instance()->createTables( m_db );

    StatusBar::instance()->newProgressOperation( this )
            .setDescription( i18n( "Building Collection" ) )
            .setAbortSlot( this, SLOT( deleteLater() ) )
            .setTotalSteps( 100 );

    m_reader.setContentHandler( this );
    m_reader.parse( &m_source, true );

    *m_scanner << "amarokcollectionscanner";
    if( AmarokConfig::importPlaylists() ) *m_scanner << "-i";
    if( AmarokConfig::scanRecursively() ) *m_scanner << "-r";
    *m_scanner << folders;

    connect( m_scanner, SIGNAL( readReady( KProcIO* ) ),      SLOT( slotReadReady() ) );
    connect( m_scanner, SIGNAL( processExited( KProcess* ) ), SLOT( slotProcessExited() ) );

    m_scanner->start();
}


ScanController::~ScanController()
{
    DEBUG_BLOCK

    m_scanner->kill();
    delete m_scanner;

    if( m_db->isConnected() ) {
        CollectionDB::instance()->dropTables( m_db );
        CollectionDB::instance()->returnStaticDbConnection( m_db );
    }

    emit CollectionDB::instance()->scanDone( m_success ); //FIXME
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

        CollectionDB::instance()->addSong( &bundle, false /*m_incremental*/, m_db );
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
        m_success = true;
        CollectionDB::instance()->clearTables();
        CollectionDB::instance()->moveTempTables( m_db ); // rename tables
    }
    else
        ::error() << "CollectionScanner has crashed! Scan aborted." << endl;

    deleteLater();
}


#include "scancontroller.moc"


