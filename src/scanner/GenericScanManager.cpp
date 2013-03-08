/****************************************************************************************
 * Copyright (c) 2003-2008 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008-2009 Jeff Mitchell <mitchell@kde.org>                             *
 * Copyright (c) 2010-2011 Ralf Engels <ralf-engels@gmx.de>                             *
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "GenericScanManager.h"

#define DEBUG_PREFIX "GenericScanManager"

#include "GenericScannerJob.h"
#include "core/support/Debug.h"

#include <threadweaver/ThreadWeaver.h>
#include <QFileInfo>

GenericScanManager::GenericScanManager( QObject *parent )
    : QObject( parent )
    , m_scannerJob( 0 )
{
    qRegisterMetaType<GenericScanManager::ScanType>( "GenericScanManager::ScanType" );
}

GenericScanManager::~GenericScanManager()
{
    abort();
}

bool
GenericScanManager::isRunning()
{
    QMutexLocker locker( &m_mutex );
    return m_scannerJob;
}

QString
GenericScanManager::getBatchFile( const QStringList& scanDirsRequested )
{
    Q_UNUSED( scanDirsRequested );
    return QString();
}

void
GenericScanManager::requestScan( QList<KUrl> directories, ScanType type )
{
    DEBUG_BLOCK;

    QMutexLocker locker( &m_mutex );
    if( m_scannerJob )
    {
        //TODO: add to queue requests
        error() << "Scanner already running";
        return;
    }

    QSet<QString> scanDirsSet;
    foreach( const KUrl &url, directories )
    {
        if( !url.isLocalFile() )
        {
            error() << "scan of non-local directory requested. Skipping.";
            continue;
        }

        QString path = url.path( KUrl::RemoveTrailingSlash );
        if( !QFileInfo( path ).isDir() )
        {
            error() << "scan of a non directory"<<path<<"requested. Skipping.";
            continue;
        }
        //TODO: most local path

        scanDirsSet << path;
    }

    if( scanDirsSet.isEmpty() )
        return; // nothing to do

    m_scannerJob = new GenericScannerJob( this, scanDirsSet.toList(), type );
    connectSignalsToJob();
    ThreadWeaver::Weaver::instance()->enqueue( m_scannerJob );
}

void
GenericScanManager::requestImport( QIODevice *input, ScanType type )
{
    QMutexLocker locker( &m_mutex );
    if( m_scannerJob )
    {
        //TODO: add to queue requests
        error() << "Scanner already running";
        return;
    }

    m_scannerJob = new GenericScannerJob( this, input, type );
    connectSignalsToJob();
    ThreadWeaver::Weaver::instance()->enqueue( m_scannerJob );
}

void
GenericScanManager::abort()
{
    QMutexLocker locker( &m_mutex );

    if( m_scannerJob )
        m_scannerJob->abort();
}

void
GenericScanManager::slotSucceeded()
{
    {
        QMutexLocker locker( &m_mutex );
        m_scannerJob = 0;
    }
    emit succeeded();
}

void
GenericScanManager::slotFailed( const QString& message )
{
    {
        QMutexLocker locker( &m_mutex );
        m_scannerJob = 0;
    }
    emit failed( message );
}

void
GenericScanManager::connectSignalsToJob()
{
    // all connections are direct connections, mainly because the
    // CollectionScanner::Directory pointer belongs to the scanner job
    // and might get missing.
    // Also multi-threading while scanning is nice.
    connect( m_scannerJob, SIGNAL(started( GenericScanManager::ScanType )),
             SIGNAL(started( GenericScanManager::ScanType )),
             Qt::DirectConnection  );
    connect( m_scannerJob, SIGNAL(directoryCount( int )),
             SIGNAL(directoryCount( int )),
             Qt::DirectConnection  );
    connect( m_scannerJob, SIGNAL(directoryScanned( CollectionScanner::Directory * )),
             SIGNAL(directoryScanned( CollectionScanner::Directory * )),
             Qt::DirectConnection );
    connect( m_scannerJob, SIGNAL(succeeded()), SLOT(slotSucceeded()),
             Qt::DirectConnection  );
    connect( m_scannerJob, SIGNAL(failed( QString )), SLOT(slotFailed( QString )),
             Qt::DirectConnection  );
}
