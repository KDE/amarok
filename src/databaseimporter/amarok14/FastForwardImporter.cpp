/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#include "FastForwardImporter.h"
#include "FastForwardImporterConfig.h"
#include "FastForwardWorker.h"
#include "CollectionManager.h"
#include "Debug.h"

#include <QSqlError>
#include <QSqlQuery>

FastForwardImporter::FastForwardImporter( QObject *parent )
    : DatabaseImporter( parent )
    , m_config( 0 )
    , m_worker( 0 )
{
}

FastForwardImporter::~FastForwardImporter()
{
    DEBUG_BLOCK
    m_worker->abort();
    m_worker->deleteLater();
}

DatabaseImporterConfig*
FastForwardImporter::configWidget( QWidget *parent )
{
    if( !m_config )
        m_config = new FastForwardImporterConfig( parent );
    return m_config;
}

void
FastForwardImporter::import()
{
    DEBUG_BLOCK
    // We've already started
    if( m_worker )
        return;

    Q_ASSERT( m_config );
    if( !m_config )
    {
        error() << "No configuration exists, bailing out of import";
        return;
    }

    m_worker = new FastForwardWorker();
    m_worker->setDriver( m_config->connectionType() );
    m_worker->setDatabaseLocation( m_config->databaseLocation() );
    m_worker->setDatabase( m_config->databaseName() );
    m_worker->setHostname( m_config->databaseHost() );
    m_worker->setUsername( m_config->databaseUser() );
    m_worker->setPassword( m_config->databasePass() );

    connect( m_worker, SIGNAL( trackAdded( Meta::TrackPtr ) ), 
             this, SIGNAL( trackAdded( Meta::TrackPtr ) ), Qt::QueuedConnection );
    connect( m_worker, SIGNAL( importError( QString ) ),
             this, SIGNAL( importError( QString ) ), Qt::QueuedConnection );
    connect( m_worker, SIGNAL( done(ThreadWeaver::Job*) ), 
             this, SLOT( finishUp() ), Qt::QueuedConnection );

    ThreadWeaver::Weaver::instance()->enqueue( m_worker );
}

void
FastForwardImporter::finishUp()
{
    DEBUG_BLOCK
    m_worker->failed() ?
        emit( importFailed() ) :
        emit( importSucceeded() );
    
    delete m_worker;
}
