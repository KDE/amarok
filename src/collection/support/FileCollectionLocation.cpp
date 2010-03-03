/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2008 Jason A. Donenfeld <Jason@zx2c4.com>                              *
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

#include "FileCollectionLocation.h"

#include "Debug.h"
#include "statusbar/StatusBar.h"

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kio/deletejob.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>

FileCollectionLocation::FileCollectionLocation() 
    : CollectionLocation()
{
    //nothing to do
}

FileCollectionLocation::~FileCollectionLocation()
{
    //nothing to do
}

bool
FileCollectionLocation::isWritable() const
{
    return true;
}

bool
FileCollectionLocation::isOrganizable() const
{
    return false;
}

bool
FileCollectionLocation::remove( const Meta::TrackPtr &track )
{
    // This block taken from SqlCollectionLocation::remove()
    DEBUG_BLOCK
    if( !track )
    {
        debug() << "track null!";
        return false;
    }

    debug() << "removing dirs for : " << track->playableUrl().path();
    // the file should be removed already, so we can clean the dirs
    bool removed = !QFile::exists( track->playableUrl().path()  );

    return removed;
}
void FileCollectionLocation::startRemoveJobs()
{
    DEBUG_BLOCK
    while ( !m_removetracks.isEmpty() )
    {
        Meta::TrackPtr track = m_removetracks.takeFirst();
        KUrl src = track->playableUrl();

        KIO::DeleteJob *job = 0;

        src.cleanPath();
        debug() << "deleting  " << src;
        KIO::JobFlags flags = KIO::HideProgressInfo;
        job = KIO::del( src, flags );
        connect( job, SIGNAL( result(KJob*) ), SLOT( slotRemoveJobFinished(KJob*) ) );
        QString name = track->prettyName();
        if( track->artist() )
            name = QString( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

        The::statusBar()->newProgressOperation( job, i18n( "Removing: %1", name ) );
        m_removejobs.insert( job, track );
    }
}

void FileCollectionLocation::slotRemoveJobFinished(KJob* job)
{
    DEBUG_BLOCK
    if( job->error() )
    {
        warning() << "An error occurred when removing a file: " << job->errorString();
        transferError( m_removejobs.value( job ), KIO::buildErrorString( job->error(), job->errorString() ) );
    }
    else
    {
        // The file is deleted, but do dir cleanup
        remove( m_removejobs.value( job ) );

        //we  assume that KIO works correctly...
        transferSuccessful( m_removejobs.value( job ) );
    }

    m_removejobs.remove( job );
    job->deleteLater();

    if(m_removejobs.isEmpty()) {
        slotRemoveOperationFinished();
    }
}


void FileCollectionLocation::removeUrlsFromCollection(const Meta::TrackList& sources)
{
    DEBUG_BLOCK
    m_removetracks = sources;

    debug() << "removing " << m_removetracks.size() << "tracks";
    startRemoveJobs();
}

#include "FileCollectionLocation.moc"
