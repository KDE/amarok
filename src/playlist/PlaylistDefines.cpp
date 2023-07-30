/****************************************************************************************
 * Copyright (c) 2010 Alexander Potashev <aspotashev@gmail.com>                         *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistDefines.h"
#include "core/meta/support/MetaConstants.h"

#include <KLocalizedString>


QStringList *Playlist::PlaylistColumnInfos::s_internalNames = nullptr;
QStringList *Playlist::PlaylistColumnInfos::s_names = nullptr;
QStringList *Playlist::PlaylistColumnInfos::s_icons = nullptr;
QList<Playlist::Column> *Playlist::PlaylistColumnInfos::s_groups = nullptr;

Playlist::PlaylistColumnInfos::PlaylistColumnInfos()
{}

const QStringList &
Playlist::PlaylistColumnInfos::internalNames()
{
    if( !s_internalNames )
    {
        // the order is important. See PlaylistDefines
        //this list is used internally and for reading writing config files and sths should not be translated!

        s_internalNames = new QStringList();
        *s_internalNames << QStringLiteral( "Placeholder" )
            << QStringLiteral( "Album" )
            << QStringLiteral( "Album artist" )
            << QStringLiteral( "Artist" )
            << QStringLiteral( "Bitrate" )
            << QStringLiteral( "Bpm" )
            << QStringLiteral( "Comment" )
            << QStringLiteral( "Composer" )
            << QStringLiteral( "Cover image" )
            << QStringLiteral( "Directory" )
            << QStringLiteral( "Disc number" )
            << QStringLiteral( "Divider" )
            << QStringLiteral( "File name" )
            << QStringLiteral( "File size" )
            << QStringLiteral( "Genre" )
            << QStringLiteral( "Group length" )
            << QStringLiteral( "Group tracks" )
            << QStringLiteral( "Labels" )
            << QStringLiteral( "Last played" )
            << QStringLiteral( "Length" )
            << QStringLiteral( "Length (seconds)" )
            << QStringLiteral( "Mood" )
            << QStringLiteral( "Moodbar" )
            << QStringLiteral( "Play count" )
            << QStringLiteral( "Rating" )
            << QStringLiteral( "Sample rate" )
            << QStringLiteral( "Score" )
            << QStringLiteral( "Source" )
            << QStringLiteral( "SourceEmblem" )
            << QStringLiteral( "Title" )
            << QStringLiteral( "Title (with track number)" )
            << QStringLiteral( "Track number" )
            << QStringLiteral( "Type" )
            << QStringLiteral( "Year" );
    }

    return *s_internalNames;
}

const QStringList &
Playlist::PlaylistColumnInfos::names()
{
    if( !s_names )
    {
        // the order is important. See PlaylistDefines
        s_names = new QStringList();
        *s_names << i18nc( "Empty placeholder token used for spacing in playlist layouts", "Placeholder" )
            << Meta::i18nForField( Meta::valAlbum )
            << Meta::i18nForField( Meta::valAlbumArtist )
            << Meta::i18nForField( Meta::valArtist )
            << Meta::i18nForField( Meta::valBitrate )
            << Meta::i18nForField( Meta::valBpm )
            << Meta::i18nForField( Meta::valComment )
            << Meta::i18nForField( Meta::valComposer )
            << i18nc( "'Cover image' playlist column name and token for playlist layouts", "Cover image" )
            << i18nc( "'Directory' playlist column name and token for playlist layouts", "Directory" )
            << Meta::i18nForField( Meta::valDiscNr )
            << i18nc( "'Divider' token for playlist layouts representing a small visual divider", "Divider" )
            << i18nc( "'File name' playlist column name and token for playlist layouts", "File name" )
            << Meta::i18nForField( Meta::valFilesize )
            << Meta::i18nForField( Meta::valGenre )
            << i18nc( "'Group length' (total play time of group) playlist column name and token for playlist layouts", "Group length" )
            << i18nc( "'Group tracks' (number of tracks in group) playlist column name and token for playlist layouts", "Group tracks" )
            << i18nc( "'Labels' playlist column name and token for playlist layouts", "Labels" )
            << Meta::i18nForField( Meta::valLastPlayed )
            << Meta::i18nForField( Meta::valLength )
            << Meta::i18nForField( Meta::valLength ) // this is length in seconds
            << i18nc( "'Mood' playlist column name and token for playlist layouts", "Mood" )
            << i18nc( "'Moodbar' playlist column name and token for playlist layouts", "Moodbar" )
            << Meta::i18nForField( Meta::valPlaycount )
            << Meta::i18nForField( Meta::valRating )
            << Meta::i18nForField( Meta::valSamplerate )
            << Meta::i18nForField( Meta::valScore )
            << i18nc( "'Source' (local collection, Magnatune.com, last.fm, ... ) playlist column name and token for playlist layouts", "Source" )
            << i18nc( "'SourceEmblem' playlist column name and token for playlist layouts", "SourceEmblem" )
            << i18nc( "'Title' (track name) playlist column name and token for playlist layouts", "Title" )
            << i18nc( "'Title (with track number)' (track name prefixed with the track number) playlist column name and token for playlist layouts", "Title (with track number)" )
            << Meta::i18nForField( Meta::valTrackNr )
            << Meta::i18nForField( Meta::valFormat )
            << Meta::i18nForField( Meta::valYear );
    }

    return *s_names;
}

const QStringList &
Playlist::PlaylistColumnInfos::icons()
{
    if( !s_icons )
    {
        // the order is important. See PlaylistDefines
        // should be kept in sync with Meta::iconForField() for shared fields
        s_icons = new QStringList();
        *s_icons << QStringLiteral("filename-space-amarok")
            << Meta::iconForField( Meta::valAlbum )
            << Meta::iconForField( Meta::valAlbumArtist )
            << Meta::iconForField( Meta::valArtist )
            << Meta::iconForField( Meta::valBitrate )
            << Meta::iconForField( Meta::valBpm )
            << Meta::iconForField( Meta::valComment )
            << Meta::iconForField( Meta::valComposer )
            << QLatin1String("") // cover image
            << QStringLiteral("folder-blue") // directory
            << Meta::iconForField( Meta::valDiscNr )
            << QStringLiteral("filename-divider")
            << QStringLiteral("filename-filetype-amarok") // filename
            << Meta::iconForField( Meta::valFilesize )
            << Meta::iconForField( Meta::valGenre )
            << QStringLiteral("filename-group-length")
            << QStringLiteral("filename-group-tracks")
            << Meta::iconForField( Meta::valLabel )
            << Meta::iconForField( Meta::valLastPlayed )
            << Meta::iconForField( Meta::valLength )
            << Meta::iconForField( Meta::valLength )
            << QLatin1String("")
            << QStringLiteral("filename-moodbar")
            << Meta::iconForField( Meta::valPlaycount )
            << Meta::iconForField( Meta::valRating )
            << Meta::iconForField( Meta::valSamplerate )
            << Meta::iconForField( Meta::valScore )
            << QStringLiteral("internet-services")
            << QLatin1String("") // source emblem
            << Meta::iconForField( Meta::valTitle )
            << Meta::iconForField( Meta::valTitle )
            << Meta::iconForField( Meta::valTrackNr )
            << Meta::iconForField( Meta::valFormat )
            << Meta::iconForField( Meta::valYear );
    }

    return *s_icons;
}


const QList<Playlist::Column> &
Playlist::PlaylistColumnInfos::groups()
{
    if( !s_groups )
    {
        s_groups = new QList<Playlist::Column>();
        *s_groups << Album
        << Artist
        << Composer
        << Directory
        << Genre
        << Rating
        << Source
        << Year;
    }
    return *s_groups;
}

bool
Playlist::isEditableColumn( Column c )
{
    return c == Album ||
        c == Artist ||
        c == Comment ||
        c == Composer ||
        c == DiscNumber ||
        c == Genre ||
        c == Rating ||
        c == Title ||
        c == TitleWithTrackNum ||
        c == TrackNumber ||
        c == Year ||
        c == Bpm;
}


bool
Playlist::isSortableColumn( Column c )
{
    return c == Album ||
        c == AlbumArtist ||
        c == Artist ||
        c == Bitrate ||
        c == Bpm ||
        c == Comment ||
        c == Composer ||
        c == Directory ||
        c == DiscNumber ||
        c == Filename ||
        c == Genre ||
        c == LastPlayed ||
        c == LengthInSeconds ||
        c == PlayCount ||
        c == Rating ||
        c == SampleRate ||
        c == Score ||
        c == Source ||
        c == Title ||
        c == TrackNumber ||
        c == Type ||
        c == Year;
}


