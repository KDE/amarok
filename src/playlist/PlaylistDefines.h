/****************************************************************************************
 * Copyright (c) 2008 - 2009 Nikolaj Hald Nielsen <nhn@kde.org>                         *
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
 * Copyright (c) 2012 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include <QString>
#include <QStringList>

namespace Playlist
{

/** A enum used by playlist and layouts to identify a token.
    We should have used the varTitle numbers for that.
*/
enum Column
{
    Shuffle = -1, // TODO: having shuffle at -1 is causing a lot of effort.
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

/**
 * A singleton class used to store translated names of playlist columns.
 * Use the global function columnNames to access them.
 *
 * @author Alexander Potashev <aspotashev@gmail.com>
 */
class PlaylistColumnInfos
{
    public:
        static const QStringList &internalNames();
        static const QStringList &names();
        static const QStringList &icons();
        static const QList<Column> &groups();

    private:
        PlaylistColumnInfos();

        static QStringList *s_internalNames;
        static QStringList *s_names;
        static QStringList *s_icons;
        static QList<Column> *s_groups;
};

inline Column columnForName( const QString &internalName )
{
    if( internalName == QLatin1String( "Shuffle" ) )
        return Shuffle;

    return static_cast<Column>(Playlist::PlaylistColumnInfos::internalNames().
                               indexOf( internalName ));
}

inline const QString &internalColumnName( Column c )
{
    return Playlist::PlaylistColumnInfos::internalNames().at( static_cast<int>(c) );
}

inline const QString &columnName( Column c )
{
    return Playlist::PlaylistColumnInfos::names().at( static_cast<int>(c) );
}

inline const QString &iconName( Column c )
{
    return Playlist::PlaylistColumnInfos::icons().at( static_cast<int>(c) );
}

inline const QList<Playlist::Column> &groupableCategories()
{
    return Playlist::PlaylistColumnInfos::groups();
}

/** these are the columns that can be directly edited by the user. */
bool isEditableColumn( Column c );

//FIXME: disabled sorting by File size, Group length, Group tracks, Length because
//       it doesn't work.
bool isSortableColumn( Column c );


}

// Q_DECLARE_METATYPE(Playlist::Column)


#endif
