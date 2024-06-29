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

#include "core/collections/CollectionLocationDelegate.h"
#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"


#include <kio/job.h>
#include <kio/deletejob.h>

#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QFileInfo>

using namespace Collections;

FileCollectionLocation::FileCollectionLocation() 
    : CollectionLocation()
{
    // nothing to do
}

FileCollectionLocation::~FileCollectionLocation()
{
    // nothing to do
}

QString
FileCollectionLocation::prettyLocation() const
{
    return QStringLiteral("File Browser Location");
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

void FileCollectionLocation::startRemoveJobs()
{
    DEBUG_BLOCK
    while ( !m_removetracks.isEmpty() )
    {
        Meta::TrackPtr track = m_removetracks.takeFirst();
        QUrl src = track->playableUrl();

        KIO::DeleteJob *job = nullptr;

        src.setPath( QDir::cleanPath(src.path()) );
        debug() << "deleting  " << src;
        KIO::JobFlags flags = KIO::HideProgressInfo;
        job = KIO::del( src, flags );
        connect( job, &KIO::Job::result, this, &FileCollectionLocation::slotRemoveJobFinished );
        QString name = track->prettyName();
        if( track->artist() )
            name = QStringLiteral( "%1 - %2" ).arg( track->artist()->name(), track->prettyName() );

        Amarok::Logger::newProgressOperation( job, i18n( "Removing: %1", name ) );
        m_removejobs.insert( job, track );
    }
}

void FileCollectionLocation::slotRemoveJobFinished( KJob *job )
{
    // ignore and error that the file did not exist in the first place. Perhaps destination
    // collection too eager? :-)
    if( job->error() && job->error() != KIO::ERR_DOES_NOT_EXIST )
        transferError( m_removejobs.value( job ), KIO::buildErrorString( job->error(), job->errorString() ) );
    else
        transferSuccessful( m_removejobs.value( job ) );

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

void FileCollectionLocation::showRemoveDialog( const Meta::TrackList &tracks )
{
    DEBUG_BLOCK
    if( !isHidingRemoveConfirm() )
    {
        Collections::CollectionLocationDelegate *delegate = Amarok::Components::collectionLocationDelegate();
        const bool del = delegate->reallyDelete( this, tracks );

        if( !del )
            abort();
        else
            slotShowRemoveDialogDone();
    } else
        slotShowRemoveDialogDone();
}

