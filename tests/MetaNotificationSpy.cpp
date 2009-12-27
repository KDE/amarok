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

#include "MetaNotificationSpy.h"

class MetaNotificationSpyPrivate : public Meta::Observer
{
public:
    MetaNotificationSpyPrivate()
        : Meta::Observer() {}

    virtual ~MetaNotificationSpyPrivate() {}

    virtual void metadataChanged( Meta::TrackPtr track ) { trackNotifications << track; }
    virtual void metadataChanged( Meta::ArtistPtr artist ) { artistNotifications << artist; }
    virtual void metadataChanged( Meta::AlbumPtr album ) { albumNotifications << album; }
    virtual void metadataChanged( Meta::GenrePtr genre ) { genreNotifications << genre; }
    virtual void metadataChanged( Meta::ComposerPtr composer ) { composerNotifications << composer; }
    virtual void metadataChanged( Meta::YearPtr year ) { yearNotifications << year; }

    Meta::TrackList trackNotifications;
    Meta::AlbumList albumNotifications;
    Meta::ArtistList artistNotifications;
    Meta::GenreList genreNotifications;
    Meta::ComposerList composerNotifications;
    Meta::YearList yearNotifications;
};

MetaNotificationSpy::MetaNotificationSpy()
    : d( new MetaNotificationSpyPrivate() )
{
//nothing to do
}

MetaNotificationSpy::MetaNotificationSpy( const Meta::TrackPtr &track )
    : d( new MetaNotificationSpyPrivate() )
{
    d->subscribeTo( track );
}

MetaNotificationSpy::MetaNotificationSpy( const Meta::AlbumPtr &album )
    : d( new MetaNotificationSpyPrivate() )
{
    d->subscribeTo( album );
}

MetaNotificationSpy::MetaNotificationSpy( const Meta::ArtistPtr &artist )
    : d( new MetaNotificationSpyPrivate() )
{
    d->subscribeTo( artist );
}

MetaNotificationSpy::MetaNotificationSpy( const Meta::ComposerPtr &composer )
    : d( new MetaNotificationSpyPrivate() )
{
    d->subscribeTo( composer );
}

MetaNotificationSpy::MetaNotificationSpy( const Meta::GenrePtr &genre )
    : d( new MetaNotificationSpyPrivate() )
{
    d->subscribeTo( genre );
}

MetaNotificationSpy::MetaNotificationSpy( const Meta::YearPtr &year )
    : d( new MetaNotificationSpyPrivate() )
{
    d->subscribeTo( year );
}

MetaNotificationSpy::~MetaNotificationSpy()
{
    delete d;
}

void
MetaNotificationSpy::subscribeTo(const Meta::AlbumPtr &album)
{
    d->subscribeTo( album );
}

void
MetaNotificationSpy::subscribeTo(const Meta::ArtistPtr &artist)
{
    d->subscribeTo( artist );
}

void
MetaNotificationSpy::subscribeTo(const Meta::ComposerPtr &composer)
{
    d->subscribeTo( composer );
}

void
MetaNotificationSpy::subscribeTo(const Meta::GenrePtr &genre)
{
    d->subscribeTo( genre );
}

void
MetaNotificationSpy::subscribeTo(const Meta::TrackPtr &track)
{
    d->subscribeTo( track );
}

void
MetaNotificationSpy::subscribeTo(const Meta::YearPtr &year)
{
    d->subscribeTo( year );
}

Meta::TrackList
MetaNotificationSpy::notificationsFromTracks() const
{
    return d->trackNotifications;
}

Meta::AlbumList
MetaNotificationSpy::notificationsFromAlbums() const
{
    return d->albumNotifications;
}

Meta::ArtistList
MetaNotificationSpy::notificationsFromArtists() const
{
    return d->artistNotifications;
}

Meta::ComposerList
MetaNotificationSpy::notificationsFromComposers() const
{
    return d->composerNotifications;
}

Meta::GenreList
MetaNotificationSpy::notificationsFromGenres() const
{
    return d->genreNotifications;
}

Meta::YearList
MetaNotificationSpy::notificationsFromYears() const
{
    return d->yearNotifications;
}
