/****************************************************************************************
 * Copyright (c) 2010 Alexander Potashev <aspotashev@gmail.com>                         *
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

#include <KLocale>

#include "PlaylistColumnNames.h"


QStringList *Playlist::PlaylistColumnNames::s_instance = 0;

Playlist::PlaylistColumnNames::PlaylistColumnNames()
{}

const QStringList &Playlist::PlaylistColumnNames::instance()
{
    if ( s_instance == 0 )
    {
        s_instance = new QStringList();
        s_instance->append( i18nc( "Empty placeholder token used for spacing in playlist layouts", "Placeholder" ) );
        s_instance->append( i18nc( "'Album' playlist column name and token for playlist layouts", "Album" ) );
        s_instance->append( i18nc( "'Album artist' playlist column name and token for playlist layouts", "Album artist" ) );
        s_instance->append( i18nc( "'Artist' playlist column name and token for playlist layouts", "Artist" ) );
        s_instance->append( i18nc( "'Bitrate' playlist column name and token for playlist layouts", "Bitrate" ) );
        s_instance->append( i18nc( "'Beats per minute' playlist column name and token for playlist layouts", "BPM" ) );
        s_instance->append( i18nc( "'Comment' playlist column name and token for playlist layouts", "Comment" ) );
        s_instance->append( i18nc( "'Composer' playlist column name and token for playlist layouts", "Composer" ) );
        s_instance->append( i18nc( "'Cover image' playlist column name and token for playlist layouts", "Cover image" ) );
        s_instance->append( i18nc( "'Directory' playlist column name and token for playlist layouts", "Directory" ) );
        s_instance->append( i18nc( "'Disc number' playlist column name and token for playlist layouts", "Disc number" ) );
        s_instance->append( i18nc( "'Divider' token for playlist layouts representing a small visual divider", "Divider" ) );
        s_instance->append( i18nc( "'File name' playlist column name and token for playlist layouts", "File name" ) );
        s_instance->append( i18nc( "'File size' playlist column name and token for playlist layouts", "File size" ) );
        s_instance->append( i18nc( "'Genre' playlist column name and token for playlist layouts", "Genre" ) );
        s_instance->append( i18nc( "'Group length' (total play time of group) playlist column name and token for playlist layouts", "Group length" ) );
        s_instance->append( i18nc( "'Group tracks' (number of tracks in group) playlist column name and token for playlist layouts", "Group tracks" ) );
        s_instance->append( i18nc( "'Labels' playlist column name and token for playlist layouts", "Labels" ) );
        s_instance->append( i18nc( "'Last played' (when was track last played) playlist column name and token for playlist layouts", "Last played" ) );
        s_instance->append( i18nc( "'Length' (track length) playlist column name and token for playlist layouts", "Length" ) );
        s_instance->append( i18nc( "'Length' (track length) playlist column name and token for playlist layouts", "Length" ) );
        s_instance->append( i18nc( "'Mood' playlist column name and token for playlist layouts", "Mood" ) );
        s_instance->append( i18nc( "'Moodbar' playlist column name and token for playlist layouts", "Moodbar" ) );
        s_instance->append( i18nc( "'Play count' playlist column name and token for playlist layouts", "Play count" ) );
        s_instance->append( i18nc( "'Rating' playlist column name and token for playlist layouts", "Rating" ) );
        s_instance->append( i18nc( "'Sample rate' playlist column name and token for playlist layouts", "Sample rate" ) );
        s_instance->append( i18nc( "'Score' playlist column name and token for playlist layouts", "Score" ) );
        s_instance->append( i18nc( "'Source' (local collection, Magnatune.com, last.fm, ... ) playlist column name and token for playlist layouts", "Source" ) );
        s_instance->append( i18nc( "'SourceEmblem' playlist column name and token for playlist layouts", "SourceEmblem" ) );
        s_instance->append( i18nc( "'Title' (track name) playlist column name and token for playlist layouts", "Title" ) );
        s_instance->append( i18nc( "'Title (with track number)' (track name prefixed with the track number) playlist column name and token for playlist layouts", "Title (with track number)" ) );
        s_instance->append( i18nc( "'Track number' playlist column name and token for playlist layouts", "Track number" ) );
        s_instance->append( i18nc( "'Type' (file format) playlist column name and token for playlist layouts", "Type" ) );
        s_instance->append( i18nc( "'Year' playlist column name and token for playlist layouts", "Year" ) );
    }

    return *s_instance;
}
