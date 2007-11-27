/*
 *  Copyright (c) 2007 Jamie Faris <farisj@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * MediaDeviceMeta.h
 *
 * This file contains definitions of Meta classes used by media device plugins.
 *
 */


#ifndef MEDIADEVICEMETA_H
#define MEDIADEVICEMETA_H

// Amarok
#include "Meta.h"

class MediaDevice;

class MediaDeviceAlbum : public Meta::Album
{
    public:
        MediaDeviceAlbum( QString name )
            : m_name( name )
        {
        }

        virtual ~MediaDeviceAlbum() {}

        // from Meta::MetaBase
        QString name() const { return m_name; }
        QString prettyName() const { return name(); }

        // from Meta::Album
        bool isCompilation() const { return false; }
        bool hasAlbumArtist() const { return false; }
        Meta::ArtistPtr albumArtist() const { return Meta::ArtistPtr(); }
        Meta::TrackList tracks() { return Meta::TrackList(); }

    private:
        QString m_name;
};


class MediaDeviceGenre : public Meta::Genre
{
    public:
        MediaDeviceGenre( QString name )
            : m_name( name )
        {
        }

        virtual ~MediaDeviceGenre() {}

        // from Meta::MetaBase
        QString name() const { return m_name; }
        QString prettyName() const { return name(); }

        // from Meta::Genre
        Meta::TrackList tracks() { return Meta::TrackList(); }

    private:
        QString m_name;
};


/**
 * A simple implementation of Meta::Artist for media devices.
 * This implementation holds it's values in member variables where
 * they can be set by the device plugin.
 *
 * Device plugins can extend this class to fetch the data directly
 * from the device or to allow editing the tags.
 */
class MediaDeviceArtist : public Meta::Artist
{
    public:
        MediaDeviceArtist( QString name )
            : m_name( name )
        {
        }

        virtual ~MediaDeviceArtist() {}

        //from Meta::MetaBase
        QString name() const { return m_name; }
        QString prettyName() const { return name(); }

        //from Meta::Artist
        Meta::TrackList tracks() { return Meta::TrackList(); }
        Meta::AlbumList albums() { return Meta::AlbumList(); }

    private:
        QString m_name;
};


/**
 * A simple implementation of Meta::Track for media devices.
 * This implementation holds it's values in member variables where
 * they can be set by the device plugin.
 *
 * Device plugins can extend this class to fetch the data directly
 * from the device and/or allow editing of the tags.
 */
class MediaDeviceTrack : public Meta::Track
{
    public:
        MediaDeviceTrack( MediaDevice *device, QString title )
            : m_device( device ),
              m_name( title )
        {
        }

        virtual ~MediaDeviceTrack() {}

        //from Meta::MetaBase
        virtual QString name() const { return m_name; }
        virtual QString prettyName() const { return name(); }

        //from Meta::Track
        virtual KUrl playableUrl() const { return KUrl(); }
        virtual QString prettyUrl() const { return QString(); }
        virtual QString url() const { return QString(); }
        virtual bool isPlayable() const { return false; }
        virtual Meta::AlbumPtr album() const { return Meta::AlbumPtr(); }
        virtual Meta::ArtistPtr artist() const { return Meta::ArtistPtr(); }
        virtual Meta::ComposerPtr composer() const { return Meta::ComposerPtr(); }
        virtual Meta::GenrePtr genre() const { return Meta::GenrePtr(); }
        virtual Meta::YearPtr year() const { return Meta::YearPtr(); }
        virtual QString comment() const { return QString(); }
        virtual double score() const { return 0; }
        virtual void setScore( double newScore ) { Q_UNUSED(newScore); return; }
        virtual int rating() const { return 0; }
        virtual void setRating( int newRating ) { Q_UNUSED(newRating); return; }
        virtual int length() const { return 0; }
        virtual int filesize() const { return 0; }
        virtual int sampleRate() const { return 0; }
        virtual int bitrate() const { return 0; }
        virtual int trackNumber() const { return 0; }
        virtual int discNumber() const { return 0; }
        virtual uint lastPlayed() const { return 0; }
        virtual uint firstPlayed() const { return 0; }
        virtual int playCount() const { return 0; }
        virtual QString type() const { return QString(); }

    private:
        MediaDevice *m_device;
        QString m_name;
};


#endif //MEDIADEVICEMETA_H
