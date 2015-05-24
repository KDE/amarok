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

#define DEBUG_PREFIX "GenericScanManager"

#include "GenericScanManager.h"

#include "core/support/Debug.h"
#include "scanner/GenericScannerJob.h"

#include <ThreadWeaver/Weaver>

#include <QFileInfo>
#include <QSharedPointer>

GenericScanManager::GenericScanManager( QObject *parent )
    : QObject( parent )
    , m_scannerJob( 0 )
{
    qRegisterMetaType<GenericScanManager::ScanType>( "GenericScanManager::ScanType" );
    qRegisterMetaType<QSharedPointer<CollectionScanner::Directory> >( "QSharedPointer<CollectionScanner::Directory>" );
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
GenericScanManager::requestScan( QList<QUrl> directories, ScanType type )
{
    DEBUG_BLOCK;

    QMutexLocker locker( &m_mutex );
    if( m_scannerJob )
    {
        //TODO: add to queue requests
        error() << "Scanner already running, not starting another instance.";
        return;
    }

    QSet<QString> scanDirsSet;
    foreach( const QUrl &url, directories )
    {
        if( !url.isLocalFile() )
        {
            warning() << "scan of non-local directory" << url << "requested, skipping it.";
            continue;
        }

        QString path = url.path( QUrl::RemoveTrailingSlash );
        if( !QFileInfo( path ).isDir() )
        {
            warning() << "scan of a non-directory" << path << "requested, skipping it.";
            continue;
        }
        //TODO: most local path

        scanDirsSet << path;
    }

    // we cannot skip the scan even for empty scanDirsSet and non-partial scan, bug 316216
    if( scanDirsSet.isEmpty() && type == PartialUpdateScan )
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
    // we used to have direct connections here, but that caused too much work being done
    // int the non-main thread, even in code that wasn't thread-safe, which lead to
    // crashes (bug 319835) and other potential data races
    connect( m_scannerJob, SIGNAL(started(GenericScanManager::ScanType)),
             SIGNAL(started(GenericScanManager::ScanType)) );
    connect( m_scannerJob, SIGNAL(directoryCount(int)), SIGNAL(directoryCount(int)) );
    connect( m_scannerJob, SIGNAL(directoryScanned(QSharedPointer<CollectionScanner::Directory>)),
             SIGNAL(directoryScanned(QSharedPointer<CollectionScanner::Directory>)) );

    connect( m_scannerJob, SIGNAL(succeeded()), SLOT(slotSucceeded()) );
    connect( m_scannerJob, SIGNAL(failed(QString)), SLOT(slotFailed(QString)) );
}
