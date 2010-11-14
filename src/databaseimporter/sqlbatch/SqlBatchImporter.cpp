/****************************************************************************************
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "SqlBatchImporter.h"
#include "SqlBatchImporterConfig.h"

#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "core-impl/collections/db/ScanManager.h"

#include <KLocale>

SqlBatchImporter::SqlBatchImporter( QObject *parent )
    : DatabaseImporter( parent )
    , m_config( 0 )
{
}

SqlBatchImporter::~SqlBatchImporter()
{
    DEBUG_BLOCK
}

DatabaseImporterConfig*
SqlBatchImporter::configWidget( QWidget *parent )
{
    if( !m_config )
        m_config = new SqlBatchImporterConfig( parent );
    return m_config;
}

void
SqlBatchImporter::import()
{
    DEBUG_BLOCK

    // as the collections are dynamically loaded I decided to go via the meta object.
/*

    Collections::SqlCollection *dbColl = 0;
    foreach( Collections::Collection *coll , CollectionManager::instance()->queryableCollections() )
    {
        dbColl = qobject_cast<Collections::SqlCollection*>(coll);
        if( dbColl )
            break;
    }

    if( !dbColl )
    {
        warning() << "No database collection found. Cannot import";
        return;
    }

    Q_ASSERT( m_config );
    if( !m_config )
    {
        error() << "No configuration exists, bailing out of import";
        return;
    }

    QString retVal;
    QMetaObject::invokeMethod(obj, "compute", Qt::DirectConnection,
                           Q_RETURN_ARG(QString, retVal),
                           Q_ARG(QString, "sqrt"),
                           Q_ARG(int, 42),
                           Q_ARG(double, 9.7));
    ScanManager *scanManager = dbColl->scanManager();
    if( !dbColl )
    {
        warning() << "The database has no scan manager";
        return;
    }

    connect( scanManager, SIGNAL( finished() ),
             this, SLOT( finishUp() ), Qt::QueuedConnection );
    connect( scanManager, SIGNAL( message( QString ) ),
             this, SIGNAL( showMessage( QString ) ), Qt::QueuedConnection );

    // scanManager->requestImport( url );
    */

/*
    connect( m_worker, SIGNAL( trackAdded( Meta::TrackPtr ) ),
             this, SIGNAL( trackAdded( Meta::TrackPtr ) ), Qt::QueuedConnection );
    connect( m_worker, SIGNAL( trackDiscarded( QString ) ),
             this, SIGNAL( trackDiscarded( QString ) ), Qt::QueuedConnection );
    connect( m_worker, SIGNAL( trackMatchFound( Meta::TrackPtr, QString ) ),
             this, SIGNAL( trackMatchFound( Meta::TrackPtr, QString ) ), Qt::QueuedConnection );
    connect( m_worker, SIGNAL( trackMatchMultiple( Meta::TrackList, QString ) ),
             this, SIGNAL( trackMatchMultiple( Meta::TrackList, QString ) ), Qt::QueuedConnection );
    connect( m_worker, SIGNAL( importError( QString ) ),
             this, SIGNAL( importError( QString ) ), Qt::QueuedConnection );
    connect( m_worker, SIGNAL( done( ThreadWeaver::Job* ) ),
             this, SLOT( finishUp() ), Qt::QueuedConnection );
    connect( m_worker, SIGNAL( showMessage( QString ) ),
             this, SIGNAL( showMessage( QString ) ), Qt::QueuedConnection );

    ThreadWeaver::Weaver::instance()->enqueue( m_worker );
    */
}

void
SqlBatchImporter::finishUp()
{
    DEBUG_BLOCK
    /*
    m_worker->failed() ?
        emit( importFailed() ) :
        emit( importSucceeded() );

    delete m_worker;
    */
}

