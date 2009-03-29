/**************************************************************************
*   Copyright (c) 2009  Casey Link <unnamedrambler@gmail.com>             *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
***************************************************************************/

#include "TimecodeWriteCapability.h"

#include "amarokurls/AmarokUrl.h"
#include "amarokurls/PlayUrlGenerator.h"
#include "amarokurls/BookmarkModel.h"
#include "Debug.h"
#include "ProgressWidget.h"
#include "EngineController.h"
#include "TimecodeLoadCapability.h"

namespace Meta
{

TimecodeWriteCapability::~TimecodeWriteCapability()
{}

bool TimecodeWriteCapability::writeTimecode( int seconds, Meta::TrackPtr track )
{
    DEBUG_BLOCK
    PlayUrlGenerator urlGenerator;
    AmarokUrl url = urlGenerator.createTrackBookmark( track, seconds );

    // lets see if we are bookmarking the currently playing track, if so
    // we need to update the slider with another icon
    Meta::TrackPtr currtrack = The::engineController()->currentTrack();
    if(  currtrack  == track )
    {
        debug() << " current track";
        ProgressWidget* pw = ProgressWidget::instance();
        if( pw )
            ProgressWidget::instance()->addBookmark( url.name(), url.arg(1).toInt() );
        else
            debug() << "ProgressWidget is NULL";
    }

    url.saveToDb();
    BookmarkModel::instance()->reloadFromDb(); //Update bookmark manager view.
    return true;
}

bool Meta::TimecodeWriteCapability::writeAutoTimecode( int seconds, Meta::TrackPtr track )
{
    DEBUG_BLOCK

    //first up, find and delete any previously added auto timecodes for this track

    debug() << "deleting old auto timecodes";
    if( track->hasCapabilityInterface( Meta::Capability::LoadTimecode ) )
    {
        TimecodeLoadCapability *tcl = track->as<TimecodeLoadCapability>();
        BookmarkList list = tcl->loadTimecodes();
        foreach( AmarokUrlPtr oldUrl, list )
        {
            if( oldUrl->command() == "play"  ) {
                if( oldUrl->customValue() == "auto timecode" ) {
                    debug() << "deleting: " << oldUrl->name();
                    oldUrl->removeFromDb();
                }
            }
        }
    }

    //create url

    PlayUrlGenerator urlGenerator;
    AmarokUrl url = urlGenerator.createTrackBookmark( track, seconds );

    // lets see if we are bookmarking the currently playing track, if so
    // we need to update the slider with another icon

    Meta::TrackPtr currtrack = The::engineController()->currentTrack();
    if( currtrack == track )
    {
        debug() << " current track";
        ProgressWidget* pw = ProgressWidget::instance();
        if( pw )
            ProgressWidget::instance()->addBookmark( url.name(), url.arg(1).toInt() );
        else
            debug() << "ProgressWidget is NULL";
    }

    //change the name a little bit
    url.setCustomValue( "auto timecode" );

    QString date = QDateTime::currentDateTime().toString( "dd.MM.yyyy" );;
    url.setName( i18n( "%1 - Playback stopped at %2" ).arg( track->prettyName() ).arg( date ) );

    debug() << "creating new auto timecode: " << url.name();

    //put in custom group to ensure that we do not clutter the list of bookmarks.
    BookmarkGroupPtr parentPtr = BookmarkGroupPtr( new BookmarkGroup( i18n( "Playback Ended Markers" ), "auto_markers" ) );
    url.reparent( parentPtr );

    //save it
    url.saveToDb();
    BookmarkModel::instance()->reloadFromDb(); //Update bookmark manager view.
    return true;
}

}




