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

#include "DatabaseImporter.h"
#include "Debug.h"

#include "amarok14/FastForwardImporter.h"
#include "itunes/ITunesImporter.h"

DatabaseImporter*
DatabaseImporterFactory::createImporter( const QString &name )
{
   if( name == FastForwardImporter::name() )
       return new FastForwardImporter();
   if( name == ITunesImporter::name() )
       return new ITunesImporter();
   return 0;
}

DatabaseImporter::DatabaseImporter()
    : m_importArtwork( false )
    , m_importing( false )
    , m_count( 0 )
{
    connect( this, SIGNAL( importFinished() ), SLOT( importingFinished() ) );
    connect( this, SIGNAL( trackAdded( Meta::TrackPtr ) ), SLOT( trackImported( Meta::TrackPtr ) ) );
}

DatabaseImporter::~DatabaseImporter()
{
}

DatabaseImporterConfig*
DatabaseImporter::configWidget( QWidget *parent )
{
    return new DatabaseImporterConfig( parent ); 
}

void
DatabaseImporter::setImportArtwork( const bool importArtwork )
{
    m_importArtwork = importArtwork;
}

bool
DatabaseImporter::importArtwork() const
{
    return m_importArtwork;
}

int
DatabaseImporter::importedCount() const
{
    return m_count;
}

void
DatabaseImporter::trackImported( Meta::TrackPtr track )
{
    Q_UNUSED( track )
    ++m_count;
}

bool
DatabaseImporter::importing() const
{
    return m_importing;
}

void
DatabaseImporter::startImporting()
{
    DEBUG_BLOCK
    m_importing = true;
    import();
}

void
DatabaseImporter::importingFinished()
{
    m_importing = false;
}

