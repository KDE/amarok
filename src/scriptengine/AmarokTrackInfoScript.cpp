/******************************************************************************
 * Copyright (C) 2008 Peter ZHOU <peterzhoulei@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "AmarokTrackInfoScript.h"

#include "App.h"
#include "EngineController.h"
#include "TheInstances.h"

#include <QtScript>

namespace Amarok
{
    AmarokTrackInfoScript::AmarokTrackInfoScript( QScriptEngine* ScriptEngine )
    : QObject( kapp )
    {
    }

    AmarokTrackInfoScript::~AmarokTrackInfoScript()
    {
    }

    int AmarokTrackInfoScript::getSampleRate()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->sampleRate() : 0;
    }

    int AmarokTrackInfoScript::getBitrate()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->bitrate() : 0;
    }

    int AmarokTrackInfoScript::getRating()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->rating() : 0;
    }

    void AmarokTrackInfoScript::setRating( int Rating )
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if ( track ) track->setRating( Rating );
    }
}

#include "AmarokTrackInfoScript.moc"
