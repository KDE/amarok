/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_METACONSTANTS_H
#define AMAROK_METACONSTANTS_H

#include <QtGlobal>

namespace Meta
{
    //track metadata
    static const qint64 valUrl          = 1LL << 0;
    static const qint64 valTitle        = 1LL << 1;
    static const qint64 valArtist       = 1LL << 2;
    static const qint64 valAlbum        = 1LL << 3;
    static const qint64 valGenre        = 1LL << 4;
    static const qint64 valComposer     = 1LL << 5;
    static const qint64 valYear         = 1LL << 6;
    static const qint64 valComment      = 1LL << 7;
    static const qint64 valTrackNr      = 1LL << 8;
    static const qint64 valDiscNr       = 1LL << 9;
    static const qint64 valBpm          = 1LL << 10;
    //track data
    static const qint64 valLength       = 1LL << 11;
    static const qint64 valBitrate      = 1LL << 12;
    static const qint64 valSamplerate   = 1LL << 13;
    static const qint64 valFilesize     = 1LL << 14;
    static const qint64 valFormat       = 1LL << 15;
    static const qint64 valCreateDate   = 1LL << 16;
    //statistics
    static const qint64 valScore        = 1LL << 17;
    static const qint64 valRating       = 1LL << 18;
    static const qint64 valFirstPlayed  = 1LL << 19;
    static const qint64 valLastPlayed   = 1LL << 20;
    static const qint64 valPlaycount    = 1LL << 21;
    static const qint64 valUniqueId     = 1LL << 22;
    //replay gain
    static const qint64 valTrackGain    = 1LL << 23;
    static const qint64 valTrackGainPeak= 1LL << 24;
    static const qint64 valAlbumGain    = 1LL << 25;
    static const qint64 valAlbumGainPeak= 1LL << 26;

    static const qint64 valAlbumArtist  = 1LL << 27;

    //custom label support
    static const qint64 valLabel        = 1LL << 28;

    namespace Field
    {
        //actual string values are not final yet
        static const QString ALBUM          = "xesam:album";
        static const QString ARTIST         = "xesam:author";
        static const QString BITRATE        = "xesam:audioBitrate";
        static const QString BPM            = "xesam:audioBPM";
        static const QString CODEC          = "xesam:audioCodec";
        static const QString COMMENT        = "xesam:comment";
        static const QString COMPOSER       = "xesam:composer";
        static const QString DISCNUMBER     = "xesam:discNumber";
        static const QString FILESIZE       = "xesam:size";
        static const QString GENRE          = "xesam:genre";
        static const QString LENGTH         = "xesam:mediaDuration";
        static const QString RATING         = "xesam:userRating";
        static const QString SAMPLERATE     = "xesam:audioSampleRate";
        static const QString TITLE          = "xesam:title";
        static const QString TRACKNUMBER    = "xesam:trackNumber";
        static const QString URL            = "xesam:url";
        static const QString YEAR           = "xesam:contentCreated";
        static const QString ALBUMGAIN      = "xesam:albumGain";
        static const QString ALBUMPEAKGAIN  = "xesam:albumPeakGain";
        static const QString TRACKGAIN      = "xesam:trackGain";
        static const QString TRACKPEAKGAIN  = "xesam:trackPeakGain";

        static const QString SCORE          = "xesam:autoRating";
        static const QString PLAYCOUNT      = "xesam:useCount";
        static const QString FIRST_PLAYED   = "xesam:firstUsed";
        static const QString LAST_PLAYED    = "xesam:lastUsed";

        static const QString UNIQUEID       = "xesam:id";
    }
}

#endif
