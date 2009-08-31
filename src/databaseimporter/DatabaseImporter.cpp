/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DatabaseImporter.h"
#include "Debug.h"

#include "amarok14/FastForwardImporter.h"
#include "itunes/ITunesImporter.h"

DatabaseImporter*
DatabaseImporterFactory::createImporter( const QString &name, QObject *parent )
{
   if( name == FastForwardImporter::name() )
       return new FastForwardImporter( parent );
   if( name == ITunesImporter::name() )
       return new ITunesImporter( parent );
   return 0;
}

DatabaseImporter::DatabaseImporter( QObject *parent )
    : QObject( parent )
    , m_importArtwork( false )
    , m_importing( false )
    , m_count( 0 )
{
    connect( this, SIGNAL( importSucceeded() ), SLOT( importingFinished() ) );
    connect( this, SIGNAL( importFailed() ), SLOT( importingFinished() ) );
    connect( this, SIGNAL( trackAdded( Meta::TrackPtr ) ), SLOT( trackImported( Meta::TrackPtr ) ) );
    connect( this, SIGNAL( trackMatchFound( Meta::TrackPtr, QString ) ), SLOT( trackMatched( Meta::TrackPtr, QString ) ) );
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

void DatabaseImporter::trackMatched( Meta::TrackPtr track, QString oldUrl )
{
    Q_UNUSED( track )
    Q_UNUSED( oldUrl )
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

