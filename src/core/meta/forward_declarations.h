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

#include "AmarokSharedPointer.h"

#include <QList>

namespace Meta
{
    class Base;
    typedef AmarokSharedPointer<Base> DataPtr;
    typedef QList<DataPtr> DataList;

    class Track;
    typedef AmarokSharedPointer<Track> TrackPtr;
    typedef QList<TrackPtr> TrackList;

    class Artist;
    typedef AmarokSharedPointer<Artist> ArtistPtr;
    typedef QList<ArtistPtr> ArtistList;

    class Album;
    typedef AmarokSharedPointer<Album> AlbumPtr;
    typedef QList<AlbumPtr> AlbumList;

    class Genre;
    typedef AmarokSharedPointer<Genre> GenrePtr;
    typedef QList<GenrePtr> GenreList;

    class Composer;
    typedef AmarokSharedPointer<Composer> ComposerPtr;
    typedef QList<ComposerPtr> ComposerList;

    class Year;
    typedef AmarokSharedPointer<Year> YearPtr;
    typedef QList<YearPtr> YearList;

    class Label;
    typedef AmarokSharedPointer<Label> LabelPtr;
    typedef QList<LabelPtr> LabelList;

    class Statistics;
    typedef AmarokSharedPointer<Statistics> StatisticsPtr;
    typedef AmarokSharedPointer<const Statistics> ConstStatisticsPtr;

    class TrackEditor;
    typedef AmarokSharedPointer<TrackEditor> TrackEditorPtr;
}

#endif // AMAROKCORE_META_FORWARD_DECLARATIONS_H
