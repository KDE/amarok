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

#define DEBUG_PREFIX "CollectionScanController"

#include "amarok.h"
#include "amarokconfig.h"
#include "collectiondb.h"
#include "debug.h"

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
        amaroK::closeOpenFiles(KProcIO::out[0],KProcIO::in[0],KProcIO::err[0]);
        return i;
    };
};


////////////////////////////////////////////////////////////////////////////////
// class CollectionScanController
////////////////////////////////////////////////////////////////////////////////

CollectionScanController::CollectionScanController( QObject* parent, QStringList folders )
    : QXmlDefaultHandler()
    , QObject( parent )
    , m_scanner( new ScannerProcIO() )
{
    DEBUG_BLOCK

    m_reader.setContentHandler( this );

    m_scanner << "amarokcollectionscanner";
    if ( AmarokConfig::importPlaylists() )
        m_scanner << "-i";
    if ( AmarokConfig::scanRecursively() )
        m_scanner << "-r";
    m_scanner << folders;

    connect( m_scanner, SIGNAL( readReady( KProcIO* ) ), this, SLOT( slotReadReady() ) );

    m_scanner->start();
}


bool CollectionScanController::startElement( const QString&, const QString &localName, const QString&, const QXmlAttributes &attrs )
{
    DEBUG_BLOCK
}


bool CollectionScanController::endElement( const QString&, const QString& localName, const QString& )
{
    DEBUG_BLOCK
}


CollectionScanController::slotReadReady()
{
    QString line;
    QString data;

    while( m_scanner->readln( line ) != -1 )
        data += line;

    m_source.setData( data );
}


#include "collectionscancontroller.moc"


