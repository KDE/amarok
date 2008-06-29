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
    double AmarokTrackInfoScript::getScore()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->score() : 0.0;
    }

    void AmarokTrackInfoScript::setScore( double Score )
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if ( track ) track->setScore( Score );
    }

    int AmarokTrackInfoScript::getInCollection()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if ( track )
            if ( track->inCollection() ) return 0;
            else return 1;
        else return -1;
    }

    QString AmarokTrackInfoScript::getType()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        QString type = track ? track->type() : QString();
        if( type == "stream/lastfm" )
            return "LastFm Stream";
        else
            return type;
    }

    int AmarokTrackInfoScript::getLength()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->length() : 0;
    }

    int AmarokTrackInfoScript::getFileSize()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->filesize() : 0;
    }

    int AmarokTrackInfoScript::getTrackNumber()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->trackNumber() : 0;
    }

    int AmarokTrackInfoScript::getDiscNumber()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->discNumber() : 0;
    }

    QString AmarokTrackInfoScript::getComment()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->comment() : QString();
    }

    int AmarokTrackInfoScript::getPlayCount()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->playCount() : 0;
    }

    int AmarokTrackInfoScript::getPlayable()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        if ( track )
            if ( track->isPlayable() ) return 0;
            else return 1;
        else return -1;
    }
    QString AmarokTrackInfoScript::getAlbum()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->album()->prettyName() : QString();
    }

    QString AmarokTrackInfoScript::getArtist()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->artist()->prettyName() : QString();
    }

    QString AmarokTrackInfoScript::getComposer()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->composer()->prettyName() : QString();
    }

    QString AmarokTrackInfoScript::getGenre()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->genre()->prettyName() : QString();
    }

    QString AmarokTrackInfoScript::getYear()
    {
        Meta::TrackPtr track = The::engineController()->currentTrack();
        return track ? track->year()->prettyName() : QString();
    }

}

#include "AmarokTrackInfoScript.moc"
