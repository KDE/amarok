/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#define DEBUG_PREFIX "TrashCollectionLocation"

#include "TrashCollectionLocation.h"

#include "core/collections/CollectionLocationDelegate.h"
#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"


#include <KIO/CopyJob>
#include <KLocalizedString>

#include <QFile>

namespace Collections {

TrashCollectionLocation::TrashCollectionLocation()
    : CollectionLocation()
    , m_trashConfirmed( false )
{
}

TrashCollectionLocation::~TrashCollectionLocation()
{
}

QString
TrashCollectionLocation::prettyLocation() const
{
    return i18n( "Trash" );
}

bool
TrashCollectionLocation::isWritable() const
{
    return true;
}

void
TrashCollectionLocation::copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                               const Transcoding::Configuration &configuration )
{
    DEBUG_BLOCK
    Q_UNUSED( configuration );

    if( sources.isEmpty() )
    {
        debug() << "Error: sources is empty";
        abort();
        return;
    }

    if( m_trashConfirmed )
    {
        QList<QUrl> files = sources.values();
        for( const QUrl &file : files )
        {
            if( !QFile::exists( file.toLocalFile() ) )
            {
                debug() << "Error: file does not exist!" << file.toLocalFile();
                abort();
                return;
            }
        }

        KIO::CopyJob *job = KIO::trash( files, KIO::HideProgressInfo );
        connect( job, &KJob::result, this, &TrashCollectionLocation::slotTrashJobFinished );

        Meta::TrackList tracks = sources.keys();
        m_trashJobs.insert( job, tracks );
        QString name = tracks.takeFirst()->prettyName();
        if( !tracks.isEmpty() )
        {
            int max = 3;
            while( !tracks.isEmpty() && (max > 0) )
            {
                name += QStringLiteral( ", %1" ).arg( tracks.takeFirst()->prettyName() );
                --max;
            }

            if( max == 0 && !tracks.isEmpty() )
                name += QLatin1String(" ...");
        }
        Amarok::Logger::newProgressOperation( job, i18n( "Moving to trash: %1", name ) );
    }
}

void
TrashCollectionLocation::showDestinationDialog( const Meta::TrackList &tracks, bool removeSources, const Transcoding::Configuration &configuration )
{
    Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
    m_trashConfirmed = delegate->reallyTrash( source(), tracks );
    if( !m_trashConfirmed )
        abort();
    else
        CollectionLocation::showDestinationDialog( tracks, removeSources, configuration );
}

void
TrashCollectionLocation::slotTrashJobFinished( KJob *job )
{
    DEBUG_BLOCK
    if( job->error() )
    {
        warning() << "An error occurred when moving a file to trash: " << job->errorString();
        for( Meta::TrackPtr track : m_trashJobs.value( job ) )
            source()->transferError( track, KIO::buildErrorString( job->error(), job->errorString() ) );
    }
    else
    {
        for( Meta::TrackPtr track : m_trashJobs.value( job ) )
            source()->transferSuccessful( track );
    }

    m_trashJobs.remove( job );
    job->deleteLater();
    if( m_trashJobs.isEmpty() )
        slotCopyOperationFinished();
}


} //namespace Collections
