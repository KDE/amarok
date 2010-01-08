/****************************************************************************************
 * Copyright (c) 2008 - 2009 Nikolaj Hald Nielsen <nhn@kde.org>                         *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

//this is the list of user visible token names and hence needs to be translated
static const QList<const char *> columnNames = ( QList<const char *>()
        << I18N_NOOP2( "Empty placeholder token used for spacing in playlist layouts", "Placeholder" )
        << I18N_NOOP2( "'Album' playlist column name and token for playlist layouts", "Album" )
        << I18N_NOOP2( "'Album artist' playlist column name and token for playlist layouts", "Album artist" )
        << I18N_NOOP2( "'Artist' playlist column name and token for playlist layouts", "Artist" )
        << I18N_NOOP2( "'Bitrate' playlist column name and token for playlist layouts", "Bitrate" )
        << I18N_NOOP2( "'Beats per minute' playlist column name and token for playlist layouts", "BPM" )
        << I18N_NOOP2( "'Comment' playlist column name and token for playlist layouts", "Comment" )
        << I18N_NOOP2( "'Composer' playlist column name and token for playlist layouts", "Composer" )
        << I18N_NOOP2( "'Cover image' playlist column name and token for playlist layouts", "Cover image" )
        << I18N_NOOP2( "'Directory' playlist column name and token for playlist layouts", "Directory" )
        << I18N_NOOP2( "'Disc number' playlist column name and token for playlist layouts", "Disc number" )
        << I18N_NOOP2( "'Divider' token for playlist layouts representing a small visual divider", "Divider" )
        << I18N_NOOP2( "'File name' playlist column name and token for playlist layouts", "File name" )
        << I18N_NOOP2( "'File size' playlist column name and token for playlist layouts", "File size" )
        << I18N_NOOP2( "'Genre' playlist column name and token for playlist layouts", "Genre" )
        << I18N_NOOP2( "'Group length' (total play time of group) playlist column name and token for playlist layouts", "Group length" )
        << I18N_NOOP2( "'Group tracks' (number of tracks in group) playlist column name and token for playlist layouts", "Group tracks" )
        << I18N_NOOP2( "'Labels' playlist column name and token for playlist layouts", "Labels" )
        << I18N_NOOP2( "'Last played' (when was track last played) playlist column name and token for playlist layouts", "Last played" )
        << I18N_NOOP2( "'Length' (track length) playlist column name and token for playlist layouts", "Length" )
        << I18N_NOOP2( "'Length' (track length) playlist column name and token for playlist layouts", "Length" )
        << I18N_NOOP2( "'Mood' playlist column name and token for playlist layouts", "Mood" )
        << I18N_NOOP2( "'Moodbar' playlist column name and token for playlist layouts", "Moodbar" )
        << I18N_NOOP2( "'Play count' playlist column name and token for playlist layouts", "Play count" )
        << I18N_NOOP2( "'Rating' playlist column name and token for playlist layouts", "Rating" )
        << I18N_NOOP2( "'Sample rate' playlist column name and token for playlist layouts", "Sample rate" )
        << I18N_NOOP2( "'Score' playlist column name and token for playlist layouts", "Score" )
        << I18N_NOOP2( "'Source' (local collection, Magnatune.com, last.fm, ... ) playlist column name and token for playlist layouts", "Source" )
        << I18N_NOOP2( "'SourceEmblem' playlist column name and token for playlist layouts", "SourceEmblem" )
        << I18N_NOOP2( "'Title' (track name) playlist column name and token for playlist layouts", "Title" )
        << I18N_NOOP2( "'Title (with track number)' (track name prefixed with the track number) playlist column name and token for playlist layouts", "Title (with track number)" )
        << I18N_NOOP2( "'Track number' playlist column name and token for playlist layouts", "Track number" )
        << I18N_NOOP2( "'Type' (file format) playlist column name and token for playlist layouts", "Type" )
        << I18N_NOOP2( "'Year' playlist column name and token for playlist layouts", "Year" ) );


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
        << "Genre"
        << "Rating"
        << "Source"
        << "Year" );

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
        << "filename-labels-amarok"
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
