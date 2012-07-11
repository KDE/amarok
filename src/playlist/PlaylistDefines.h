/****************************************************************************************
 * Copyright (c) 2008 - 2009 Nikolaj Hald Nielsen <nhn@kde.org>                         *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef AMAROK_PLAYLISTDEFINES_H
#define AMAROK_PLAYLISTDEFINES_H

#include <KLocale>

#include <QString>
#include <QStringList>

namespace Playlist
{

enum Column
{
    PlaceHolder = 0,
    Album,
    AlbumArtist,
    Artist,
    Bitrate,
    Bpm,
    Comment,
    Composer,
    CoverImage,
    Directory,
    DiscNumber,
    Divider,
    Filename,
    Filesize,
    Genre,
    GroupLength,
    GroupTracks,
    Labels,
    LastPlayed,
    Length,
    LengthInSeconds,
    Mood,
    Moodbar,
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
//when sorting, Random is -1

//these are the columns that can be directly edited by the user.
static const QList<int> editableColumns = ( QList<int>() )
        << Album
        << Artist
        << Comment
        << Composer
        << DiscNumber
        << Genre
        << Rating
        << Title
        << TitleWithTrackNum
        << TrackNumber
        << Year
        << Bpm;

//this list is used internally and for reading writing config files and sths should not be translated!
//must be kept in sync with the above list though!
static const QStringList internalColumnNames = ( QStringList()
        << "Placeholder"
        << "Album"
        << "Album artist"
        << "Artist"
        << "Bitrate"
        << "Bpm"
        << "Comment"
        << "Composer"
        << "Cover image"
        << "Directory"
        << "Disc number"
        << "Divider"
        << "File name"
        << "File size"
        << "Genre"
        << "Group length"
        << "Group tracks"
        << "Labels"
        << "Last played"
        << "Length"
        << "Length (seconds)"
        << "Mood"
        << "Moodbar"
        << "Play count"
        << "Rating"
        << "Sample rate"
        << "Score"
        << "Source"
        << "SourceEmblem"
        << "Title"
        << "Title (with track number)"
        << "Track number"
        << "Type"
        << "Year" );

//FIXME: disabled sorting by File size, Group length, Group tracks, Length because
//       it doesn't work.
static const QStringList sortableCategories = ( QStringList()
        << "Album"
        << "Album artist"
        << "Artist"
        << "Bitrate"
        << "Bpm"
        << "Comment"
        << "Composer"
        << "Directory"
        << "Disc number"
        << "File name"
        << "Genre"
        << "Last played"
        << "Length (seconds)"
        << "Play count"
        << "Rating"
        << "Sample rate"
        << "Score"
        << "Source"
        << "Title"
        << "Track number"
        << "Type"
        << "Year" );

static const QStringList groupableCategories = ( QStringList()
        << "Album"
        << "Artist"
        << "Composer"
        << "Directory"
        << "Genre"
        << "Rating"
        << "Source"
        << "Year" );

// should be kept in sync with Meta::iconForField() for shared fields
static const QStringList iconNames = ( QStringList()
        << "filename-space-amarok"
        << "filename-album-amarok"
        << "filename-artist-amarok"
        << "filename-artist-amarok"
        << "application-octet-stream"
        << "filename-bpm-amarok"
        << "filename-comment-amarok"
        << "filename-composer-amarok"
        << ""
        << "folder-blue"
        << "filename-discnumber-amarok"
        << "filename-divider"
        << "filename-filetype-amarok"
        << "help-about"
        << "filename-genre-amarok"
        << "filename-group-length"
        << "filename-group-tracks"
        << "label-amarok"
        << "filename-last-played"
        << "chronometer"
        << "chronometer"
        << ""
        << "filename-moodbar"
        << "amarok_playcount"
        << "rating"
        << "filename-sample-rate"
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
    MatchYear = 32,
    MatchRating = 64
};

enum DataRoles
{
    TrackRole = Qt::UserRole,
    StateRole,
    UniqueIdRole,
    ActiveTrackRole,
    QueuePositionRole,
    InCollectionRole,
    MultiSourceRole,
    StopAfterTrackRole
};

}

#endif
