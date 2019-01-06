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

#include "core/amarokcore_export.h"
#include "MetaValues.h"
#include "core/meta/forward_declarations.h"

#include <QString>

namespace Meta
{
    /** Returns a textual identification for the given field.
        This name can be used e.g. for identifying the field in a xml file.
     */
    AMAROK_CORE_EXPORT QString nameForField( qint64 field );

    /** The inverse of nameForField
     */
    AMAROK_CORE_EXPORT qint64 fieldForName( const QString &name );

    /** Returns a localized name for the given field.
     */
    AMAROK_CORE_EXPORT QString i18nForField( qint64 field );

    /** Returns a short localized name for the given field.
        The short form is only one word and is used for the collection filter.
        e.g. the "added to collection" is just "added"
     */
    AMAROK_CORE_EXPORT QString shortI18nForField( qint64 field );

    /** Returns a textual identification for the given field.
        This name is used in the playlist generator and is slightly different from
        the one in nameForField
     */
    AMAROK_CORE_EXPORT QString playlistNameForField( qint64 field );

    /** The inverse of playlistNameForField
     */
    AMAROK_CORE_EXPORT qint64 fieldForPlaylistName( const QString &name );

    /** Returns the name of the icon representing the field.
        May return an empty string if no such icon exists.
        Create the icon with QIcon::fromTheme(iconForField(field))
     */
    AMAROK_CORE_EXPORT QString iconForField( qint64 field );

    /** Returns the value for the given field.
     */
    AMAROK_CORE_EXPORT QVariant valueForField( qint64 field, TrackPtr track );

    /**
     * The Field variables. Please note that these constants are considered deprecated.
     * Use Meta::val* (e.g. Meta::valArtist, Meta::valAlbum) constants instead of these
     * in new code unless you have to.
     */
    namespace Field
    {
        //actual string values are not final yet
        static const QString ALBUM          = QStringLiteral("xesam:album");
        static const QString ARTIST         = QStringLiteral("xesam:author");
        static const QString BITRATE        = QStringLiteral("xesam:audioBitrate");
        static const QString BPM            = QStringLiteral("xesam:audioBPM");
        static const QString CODEC          = QStringLiteral("xesam:audioCodec");
        static const QString COMMENT        = QStringLiteral("xesam:comment");
        static const QString COMPOSER       = QStringLiteral("xesam:composer");
        static const QString DISCNUMBER     = QStringLiteral("xesam:discNumber");
        static const QString FILESIZE       = QStringLiteral("xesam:size");
        static const QString GENRE          = QStringLiteral("xesam:genre");
        static const QString LENGTH         = QStringLiteral("xesam:mediaDuration");
        static const QString RATING         = QStringLiteral("xesam:userRating");
        static const QString SAMPLERATE     = QStringLiteral("xesam:audioSampleRate");
        static const QString TITLE          = QStringLiteral("xesam:title");
        static const QString TRACKNUMBER    = QStringLiteral("xesam:trackNumber");
        static const QString URL            = QStringLiteral("xesam:url");
        static const QString YEAR           = QStringLiteral("xesam:contentCreated");
        static const QString ALBUMARTIST    = QStringLiteral("xesam:albumArtist");
        static const QString ALBUMGAIN      = QStringLiteral("xesam:albumGain");
        static const QString ALBUMPEAKGAIN  = QStringLiteral("xesam:albumPeakGain");
        static const QString TRACKGAIN      = QStringLiteral("xesam:trackGain");
        static const QString TRACKPEAKGAIN  = QStringLiteral("xesam:trackPeakGain");

        static const QString SCORE          = QStringLiteral("xesam:autoRating");
        static const QString PLAYCOUNT      = QStringLiteral("xesam:useCount");
        static const QString FIRST_PLAYED   = QStringLiteral("xesam:firstUsed");
        static const QString LAST_PLAYED    = QStringLiteral("xesam:lastUsed");

        static const QString UNIQUEID       = QStringLiteral("xesam:id");

        // new
        static const QString COMPILATION    = QStringLiteral("xesam:compilation");
    }
}

#endif
