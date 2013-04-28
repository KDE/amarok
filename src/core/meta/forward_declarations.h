/****************************************************************************************
 * Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef AMAROKCORE_META_FORWARD_DECLARATIONS_H
#define AMAROKCORE_META_FORWARD_DECLARATIONS_H

#include <KSharedPtr>

#include <QList>

namespace Meta
{
    class Base;
    typedef KSharedPtr<Base> DataPtr;
    typedef QList<DataPtr> DataList;

    class Track;
    typedef KSharedPtr<Track> TrackPtr;
    typedef QList<TrackPtr> TrackList;

    class Artist;
    typedef KSharedPtr<Artist> ArtistPtr;
    typedef QList<ArtistPtr> ArtistList;

    class Album;
    typedef KSharedPtr<Album> AlbumPtr;
    typedef QList<AlbumPtr> AlbumList;

    class Genre;
    typedef KSharedPtr<Genre> GenrePtr;
    typedef QList<GenrePtr> GenreList;

    class Composer;
    typedef KSharedPtr<Composer> ComposerPtr;
    typedef QList<ComposerPtr> ComposerList;

    class Year;
    typedef KSharedPtr<Year> YearPtr;
    typedef QList<YearPtr> YearList;

    class Label;
    typedef KSharedPtr<Label> LabelPtr;
    typedef QList<LabelPtr> LabelList;

    class Statistics;
    typedef KSharedPtr<Statistics> StatisticsPtr;
    typedef KSharedPtr<const Statistics> ConstStatisticsPtr;
}

#endif // AMAROKCORE_META_FORWARD_DECLARATIONS_H
