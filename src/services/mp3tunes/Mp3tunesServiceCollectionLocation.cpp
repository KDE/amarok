/****************************************************************************************
 * Copyright (c) 2008 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "Mp3tunesServiceCollectionLocation.h"

#include "Mp3tunesWorkers.h"
#include "statusbar/StatusBar.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include "Debug.h"
using namespace Meta;

Mp3tunesServiceCollectionLocation::Mp3tunesServiceCollectionLocation( Mp3tunesServiceCollection const *parentCollection )
    : ServiceCollectionLocation()
    , m_collection( const_cast<Mp3tunesServiceCollection*>(  parentCollection ) )
{
    DEBUG_BLOCK
}

Mp3tunesServiceCollectionLocation::~Mp3tunesServiceCollectionLocation()
{
}

QString Mp3tunesServiceCollectionLocation::prettyLocation() const
{
    return i18n( "MP3tunes Locker" );
}

bool Mp3tunesServiceCollectionLocation::isWritable() const
{
    return true;
}

bool Mp3tunesServiceCollectionLocation::remove( const Meta::TrackPtr &/*track*/ )
{
    //TODO
    return false;
}
void Mp3tunesServiceCollectionLocation::copyUrlsToCollection (
        const QMap<Meta::TrackPtr, KUrl> &sources )
{
    DEBUG_BLOCK
    QStringList urls;
    QString error;
    debug() << "sources has " << sources.count();
    foreach( const Meta::TrackPtr &track, sources.keys() )
    {
        debug() << "copying " << sources[ track ] << " to mp3tunes locker";
        debug() << "file is at " << sources[ track ].pathOrUrl();

        QString supported_types = "mp3 mp4 m4a m4p aac wma ogg";
        
        if( supported_types.contains( track->type() ) )
        {   

            debug() << "Added " << sources[ track ].pathOrUrl() << " to queue.";
            urls.push_back( sources[ track ].pathOrUrl() );

        } 
        else 
        {
            error = i18n( "Only the following types of tracks can be uploaded to MP3tunes: mp3, mp4, m4a, m4p, aac, wma, and ogg. " );
            debug() << "File type not supprted " << track->type();
        }
    }
    if( !error.isEmpty() )
        The::statusBar()->longMessage( error );
    Mp3tunesSimpleUploader * uploadWorker = new Mp3tunesSimpleUploader( m_collection->locker(), urls );
    connect( uploadWorker, SIGNAL( uploadComplete() ), this, SLOT( slotCopyOperationFinished() ) );
    ThreadWeaver::Weaver::instance()->enqueue( uploadWorker );
}
