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
            : m_name( name ),
              m_isCompilation( false )
        {
        }

        virtual ~MediaDeviceAlbum() {}

        // from Meta::MetaBase
        QString name() const { return m_name; }
        QString prettyName() const { return name(); }

        // from Meta::Album
        bool isCompilation() const { return m_isCompilation; }
        void setCompilation( bool isCompilation ) { m_isCompilation = isCompilation; }
        bool hasAlbumArtist() const { return false; }
        Meta::ArtistPtr albumArtist() const { return Meta::ArtistPtr(); }
        Meta::TrackList tracks() { return Meta::TrackList(); }

    private:
        QString m_name;
        bool m_isCompilation;
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
              m_name( title ),
              m_comment( QString() ),
              m_score( 0.0 ),
              m_rating( 0 ),
              m_length( 0 ),
              m_filesize( 0 ),
              m_samplerate( 0 ),
              m_bitrate( 0 ),
              m_trackNumber( 0 ),
              m_discNumber( 0 ),
              m_lastPlayed( 0 ),
              m_firstPlayed( 0 ),
              m_playCount( 0 ),
              m_type( QString() )
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
        virtual QString comment() const { return m_comment; }
        virtual void setComment( QString comment ) { m_comment = comment; }
        virtual double score() const { return 0; }
        virtual void setScore( double score ) { m_score = score; }
        virtual int rating() const { return 0; }
        virtual void setRating( int rating ) { m_rating = rating; }
        virtual int length() const { return 0; }
        virtual void setLength( int length ) { m_length = length; }
        virtual int filesize() const { return 0; }
        virtual void setFileSize( int size ) { m_filesize = size; }
        virtual int sampleRate() const { return 0; }
        virtual void setSampleRate( int rate ) { m_samplerate = rate; }
        virtual int bitrate() const { return 0; }
        virtual void setBitrate( int rate ) { m_bitrate = rate; }
        virtual int trackNumber() const { return 0; }
        virtual void setTrackNumber( int number ) { m_trackNumber = number; }
        virtual int discNumber() const { return 0; }
        virtual void setDiscNumber( int number ) { m_discNumber = number; }
        virtual uint lastPlayed() const { return 0; }
        virtual void setLastPlayed( uint last ) { m_lastPlayed = last; }
        virtual uint firstPlayed() const { return 0; }
        virtual void setFirstPlayed( uint first ) { m_firstPlayed = first; }
        virtual int playCount() const { return 0; }
        virtual void setPlayCount( int count ) { m_playCount = count; }
        virtual QString type() const { return m_type; }
        virtual void setType( QString type ) { m_type = type; }


    private:
        MediaDevice *m_device;
        QString m_name;
        QString m_comment;
        double m_score;
        int m_rating;
        int m_length;
        int m_filesize;
        int m_samplerate;
        int m_bitrate;
        int m_trackNumber;
        int m_discNumber;
        uint m_lastPlayed;
        uint m_firstPlayed;
        int m_playCount;
        QString m_type;
};


#endif //MEDIADEVICEMETA_H
