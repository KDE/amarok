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
#include "scancontroller.h"

#include <kprocio.h>

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
    , m_scanner( new ScannerProcIO() )
{
    DEBUG_BLOCK

    m_reader.setContentHandler( this );
    m_reader.parse( &m_source, true );

    *m_scanner << "amarokcollectionscanner";
    if ( AmarokConfig::importPlaylists() )
        *m_scanner << "-i";
    if ( AmarokConfig::scanRecursively() )
        *m_scanner << "-r";
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
}


bool
ScanController::startElement( const QString&, const QString& localName, const QString&, const QXmlAttributes& attrs )
{
    DEBUG_BLOCK

    debug() << "localName: " << localName << endl;
    debug() << "title    : " << attrs.value( "title" ) << endl;
    debug() << "artist   : " << attrs.value( "artist" ) << endl;
    debug() << "album    : " << attrs.value( "album" ) << endl;
    debug() << "comment  : " << attrs.value( "comment" ) << endl;
    debug() << endl;

    return true;
}


void
ScanController::slotReadReady()
{
    DEBUG_BLOCK

    QString line, data;
    bool partial;

    while( m_scanner->readln( line, true, &partial ) != -1 )
        data += line;

    m_source.setData( data );

    if( !m_reader.parseContinue() )
        ::warning() << "parseContinue() failed: " << errorString() << endl;
}


void
ScanController::slotProcessExited()
{
    DEBUG_BLOCK

    if( !m_scanner->normalExit() )
        ::error() << "CollectionScanner has crashed." << endl;

    deleteLater();
}


#include "scancontroller.moc"


