/****************************************************************************************
 * Copyright (c) 2009 Casey Link <unnamedrambler@gmail.com>                             *
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


#include "TimecodeWriteCapability.h"

#include "EngineController.h"
#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlHandler.h"
#include "amarokurls/BookmarkModel.h"
#include "amarokurls/PlayUrlGenerator.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/capabilities/timecode/TimecodeLoadCapability.h"

#include <KLocalizedString>

namespace Capabilities
{

TimecodeWriteCapability::~TimecodeWriteCapability()
{}

bool TimecodeWriteCapability::writeTimecode( qint64 miliseconds, const Meta::TrackPtr &track )
{
    DEBUG_BLOCK
    AmarokUrl url = PlayUrlGenerator::instance()->createTrackBookmark( track, miliseconds );

    // lets see if we are bookmarking the currently playing track, if so
    // we need to update the slider with another icon
    Meta::TrackPtr currtrack = The::engineController()->currentTrack();
    if( currtrack  == track )
    {
        debug() << " current track";
        debug() << "adding at seconds: " << miliseconds;
        The::amarokUrlHandler()->paintNewTimecode( url.name(), miliseconds );

    }

    url.saveToDb();
    BookmarkModel::instance()->reloadFromDb(); //Update bookmark manager view.
    return true;
}

bool Capabilities::TimecodeWriteCapability::writeAutoTimecode( qint64 miliseconds, Meta::TrackPtr track )
{
    DEBUG_BLOCK

    //first up, find and delete any previously added auto timecodes for this track

    debug() << "deleting old auto timecodes";
    if( track->has<Capabilities::TimecodeLoadCapability>() )
    {
        QScopedPointer<TimecodeLoadCapability> tcl( track->create<TimecodeLoadCapability>() );
        BookmarkList list = tcl->loadTimecodes();
        for( AmarokUrlPtr oldUrl : list )
        {
            if( oldUrl->command() == QLatin1String("play")  ) {
                if( oldUrl->customValue() == QLatin1String("auto timecode") ) {
                    debug() << "deleting: " << oldUrl->name();
                    oldUrl->removeFromDb();
                }
            }
        }
    }

    //create url
    AmarokUrl url = PlayUrlGenerator::instance()->createTrackBookmark( track, miliseconds );

    // lets see if we are bookmarking the currently playing track, if so
    // we need to update the slider with another icon

    Meta::TrackPtr currtrack = The::engineController()->currentTrack();
    if( currtrack == track )
    {
        debug() << " current track";
        QMap<QString, QString> args = url.args();
        if ( args.keys().contains( QStringLiteral("pos") ) )
        {
            int pos = args.value( QStringLiteral("pos") ).toInt();
            The::amarokUrlHandler()->paintNewTimecode( url.name(), pos * 1000 );
        }
    }

    //change the name a little bit
    url.setCustomValue( QStringLiteral("auto timecode") );

    QString date = QDateTime::currentDateTime().toString( QStringLiteral("dd.MM.yyyy") );
    url.setName( i18n( "%1 - Stopped %2", track->prettyName(), date ) );

    debug() << "creating new auto timecode: " << url.name();

    //put in custom group to ensure that we do not clutter the list of bookmarks.
    BookmarkGroupPtr parentPtr = BookmarkGroupPtr( new BookmarkGroup( i18n( "Playback Ended Markers" ), QStringLiteral("auto_markers") ) );
    url.reparent( parentPtr );

    //save it
    url.saveToDb();
    BookmarkModel::instance()->reloadFromDb(); //Update bookmark manager view.
    return true;
}

}




