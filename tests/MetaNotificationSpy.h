/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#ifndef METANOTIFICATIONSPY_H
#define METANOTIFICATIONSPY_H

#include "core/meta/Meta.h"

class MetaNotificationSpyPrivate;

class MetaNotificationSpy
{
public:
    explicit MetaNotificationSpy();
    explicit MetaNotificationSpy( const Meta::TrackPtr &track );
    explicit MetaNotificationSpy( const Meta::ArtistPtr &artist );
    explicit MetaNotificationSpy( const Meta::AlbumPtr &album );
    explicit MetaNotificationSpy( const Meta::GenrePtr &genre );
    explicit MetaNotificationSpy( const Meta::ComposerPtr &composer );
    explicit MetaNotificationSpy( const Meta::YearPtr &year );
    ~ MetaNotificationSpy();

    void subscribeTo( const Meta::TrackPtr &track );
    void subscribeTo( const Meta::AlbumPtr &album );
    void subscribeTo( const Meta::ArtistPtr &artist );
    void subscribeTo( const Meta::GenrePtr &genre );
    void subscribeTo( const Meta::YearPtr &year );
    void subscribeTo( const Meta::ComposerPtr &composer );

    Meta::TrackList notificationsFromTracks() const;
    Meta::AlbumList notificationsFromAlbums() const;
    Meta::ArtistList notificationsFromArtists() const;
    Meta::GenreList notificationsFromGenres() const;
    Meta::YearList notificationsFromYears() const;
    Meta::ComposerList notificationsFromComposers() const;

private:
    MetaNotificationSpyPrivate * const d;
};

#endif // METANOTIFICATIONSPY_H
