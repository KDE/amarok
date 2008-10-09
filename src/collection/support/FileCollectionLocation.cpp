/*
 *  Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "FileCollectionLocation.h"

#include "Debug.h"
#include "MountPointManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

FileCollectionLocation::FileCollectionLocation() 
    : CollectionLocation()
    , m_removeSources( false )
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
    return false;
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
        return false;
    bool removed;
    if( m_tracksRemovedByDestination.contains( track ) )
    {
        removed = true;
    }
    else
    {
        removed = QFile::remove( track->playableUrl().path() );
    }
    if( removed ) 
    {
        QFileInfo file( track->playableUrl().path() );
        QDir dir = file.dir();
        const QStringList collectionFolders = MountPointManager::instance()->collectionFolders();
        while( !collectionFolders.contains( dir.absolutePath() ) && !dir.isRoot() && dir.count() == 0 )
        {
            const QString name = dir.dirName();
            dir.cdUp();
            if( !dir.rmdir( name ) )
                break;
        }

    }
    return removed;
}

#include "FileCollectionLocation.moc"
