/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#ifndef AMAROK_PLAYLISTDEFINES_H
#define AMAROK_PLAYLISTDEFINES_H

#include <KLocale>

#include <QString>
#include <QStringList>

namespace Playlist
{

enum Column
{
    Album = 1,
    AlbumArtist,
    Artist,
    Bitrate,
    Bpm,
    Comment,
    Composer,
    CoverImage,
    Directory,
    DiscNumber,
    Filename,
    Filesize,
    Genre,
    LastPlayed,
    Length,
    Mood,
    PlayCount,
    Rating,
    SampleRate,
    Score,
    Source,
    SourceEmblem,
    Title,
    TitleWithTrackNum,
    TrackNumber,
    Type,
    Year,
    NUM_COLUMNS
};

static const QStringList columnNames = ( QStringList()
        << "DUMMY_VALUE"
        << i18n( "Album" )
        << i18n( "Album artist" )
        << i18n( "Artist" )
        << i18n( "Bitrate" )
        << i18n( "Bpm" )
        << i18n( "Comment" )
        << i18n( "Composer" )
        << i18n( "Cover image" )
        << i18n( "Directory" )
        << i18n( "Disc number" )
        << i18n( "File name" )
        << i18n( "File size" )
        << i18n( "Genre" )
        << i18n( "Last played" )
        << i18n( "Length" )
        << i18n( "Mood" )
        << i18n( "Play count" )
        << i18n( "Rating" )
        << i18n( "Sample rate" )
        << i18n( "Score" )
        << i18n( "Source" )
        << i18n( "SourceEmblem" )
        << i18n( "Title" )
        << i18n( "Title (with track number)" )
        << i18n( "Track number" )
        << i18n( "Type" )
        << i18n( "Year" ) );

static const QStringList iconNames = ( QStringList()
        << "DUMMY_VALUE"
        << "filename-album-amarok"
        << "filename-album-amarok"
        << "filename-artist-amarok"
        << "application-octet-stream"
        << ""
        << "filename-comment-amarok"
        << "filename-composer-amarok"
        << ""
        << ""
        << "filename-discnumber-amarok"
        << ""
        << ""
        << "filename-genre-amarok"
        << ""
        << "chronometer"
        << ""
        << ""
        << "rating"
        << ""
        << "emblem-favorite"
        << "applications-internet"
        << ""
        << "filename-title-amarok"
        << "filename-title-amarok"
        << "filename-track-amarok"
        << "filename-filetype-amarok"
        << "filename-year-amarok" );


enum SearchFields
{
    MatchTrack = 1,
    MatchArtist = 2,
    MatchAlbum = 4,
    MatchGenre = 8,
    MatchComposer = 16,
    MatchYear = 32
};


}

#endif
