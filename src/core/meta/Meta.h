/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_META_H
#define AMAROK_META_H

#include "MetaReplayGain.h"
#include "core/meta/Base.h"

#include <QList>
#include <QMutex>
#include <QImage>
#include <QDateTime>
#include <QSharedData>

#include <QUrl>

namespace Collections
{
    class Collection;
    class QueryMaker;
}
class PersistentStatisticsStore;

namespace Meta
{
    class AMAROK_CORE_EXPORT Track : public Base
    {
        public:
            /** used to display the trackname, should never be empty, returns prettyUrl() by default if name() is empty */
            virtual QString prettyName() const;
            /** an url which can be played by the engine backends */
            virtual QUrl playableUrl() const = 0;
            /** an url for display purposes */
            virtual QString prettyUrl() const = 0;

            /**
             * A fake url which is unique for this track. Use this if you need a key for
             * the track.
             */
            virtual QString uidUrl() const = 0;

            /**
             * Return whether playableUrl() will return a valid & readable playable url.
             *
             * Convenience method that just checks whether notPlayableReason() is empty.
             */
            bool isPlayable() const;

            /**
             * Return user-readable localized reason why isPlayable() is false.
             *
             * Subclasses must return a non-empty (localized) string if this track is not
             * playable (i.e. playableUrl() won't return a valid url) and an empty string
             * otherwise.
             *
             * This method is used to implement convenience Meta::Track::isPlayable()
             * method.
             */
            virtual QString notPlayableReason() const = 0;

            /** Returns the album this track belongs to */
            virtual AlbumPtr album() const = 0;
            /** Returns the artist of this track */
            virtual ArtistPtr artist() const = 0;
            /** Returns the composer of this track */
            virtual ComposerPtr composer() const = 0;
            /** Returns the genre of this track */
            virtual GenrePtr genre() const = 0;
            /** Returns the year of this track */
            virtual YearPtr year() const = 0;
            /**
              * Returns the labels that are assigned to a track.
              */
            virtual Meta::LabelList labels() const;
            /** Returns the BPM of this track */
            virtual qreal bpm() const = 0;
            /** Returns the comment of this track */
            virtual QString comment() const = 0;
            /** Returns the length of this track in milliseconds, or 0 if unknown */
            virtual qint64 length() const = 0;
            /** Returns the filesize of this track in bytes */
            virtual int filesize() const = 0;
            /** Returns the sample rate of this track */
            virtual int sampleRate() const = 0;
            /** Returns the bitrate o this track in kbps (kilo BITS per second) */
            virtual int bitrate() const = 0;
            /** Returns the time when the track was added to the collection,
                or an invalid QDateTime if the time is not known. */
            virtual QDateTime createDate() const;
            /** Returns the time when the track was last modified (before being added to the collection)
                or an invalid QDateTime if the time is not known. */
            virtual QDateTime modifyDate() const;
            /** Returns the track number of this track */
            virtual int trackNumber() const = 0;
            /** Returns the discnumber of this track */
            virtual int discNumber() const = 0;
            /**
             * Returns the gain adjustment for a given replay gain mode.
             *
             * Should return @c 0 if no replay gain value is known.
             *
             * Should return the track replay gain if the album gain is requested but
             * is not available.
             */
            virtual qreal replayGain( ReplayGainTag mode ) const;

            /**
             * Returns the type of this track, e.g. "ogg", "mp3", "stream"
             *
             * TODO: change return type to Amarok::FileType enum. Clients needing
             * user-representation would call FileTypeSupport::toLocalizedString()
             */
            virtual QString type() const = 0;

            /** tell the track to perform any prerequisite
             * operations before playing */
            virtual void prepareToPlay();

            /** tell the track object that amarok finished playing it.
                The argument is the percentage of the track which was played, in the range 0 to 1*/
            virtual void finishedPlaying( double playedFraction );

            /** returns true if the track is part of a collection false otherwise */
            virtual bool inCollection() const;
            /**
                returns the collection that the track is part of, or 0 iff
                inCollection() returns false */
            virtual Collections::Collection* collection() const;

            /** get the cached lyrics for the track. returns an empty string if
                no cached lyrics are available */
            virtual QString cachedLyrics() const;
            /**
                set the cached lyrics for the track
            */
            virtual void setCachedLyrics( const QString &lyrics );

            /**
              Adds a label to the track.
              Does nothing if the track already has the given label.
             */
            virtual void addLabel( const QString &label );
            /**
              Adds a label to the track.
              Does nothing if the track already has the given label.
             */
            virtual void addLabel( const Meta::LabelPtr &label );

            /**
              Removes a lbel from a track.
              Does nothing if the track does not actually have the label assigned to it.
             */
            virtual void removeLabel( const Meta::LabelPtr &label );

            virtual bool operator==( const Track &track ) const;

            static bool lessThan( const TrackPtr& left, const TrackPtr& right );

            /**
             * Return a pointer to TrackEditor interface that allows you to edit metadata
             * of this track. May be null, which signifies that the track is not editable.
             *
             * This is a replacement to \::create<Capabilities::EditCapability>() with more
             * well-defined memory management and nicer implementation possibilities.
             * (multiple inheritance and returning self)
             *
             * Default implementation returns null pointer.
             */
            virtual TrackEditorPtr editor();

            /**
             * Return a pointer to track's Statistics interface. May never be null.
             *
             * Subclasses: always return the default implementation instead of returning 0.
             */
            virtual StatisticsPtr statistics();
            ConstStatisticsPtr statistics() const; // allow const statistics methods on const tracks

        protected:
            friend class ::PersistentStatisticsStore; // so that it can call notifyObservers
            virtual void notifyObservers() const;

            /**
             * Helper method for subclasses to implement notPlayableReason().
             * Checks network status and returns a non-empty reason string if
             * it isn't online.
             */
            QString networkNotPlayableReason() const;

            /**
             * Helper method for subclasses to implement notPlayableReason().
             * Checks, in order, if the file exists, if it is a file and if
             * the file is readable
             */
            QString localFileNotPlayableReason( const QString &path ) const;
    };

    class AMAROK_CORE_EXPORT Artist : public Base
    {
        public:
            virtual QString prettyName() const;

            /** returns all tracks by this artist */
            virtual TrackList tracks() = 0;

            virtual bool operator==( const Meta::Artist &artist ) const;

            virtual QString sortableName() const;

        protected:
            virtual void notifyObservers() const;

        private:
            mutable QString m_sortableName;
    };

    /**
     * Represents an album.
     *
     * Most collections do not store a specific album object. Instead an album is just
     * a property of a track, a container containing one or more tracks.
     *
     * Collections should provide an album for every track as the collection browser
     * will, depending on the setting, only display tracks inside albums.
     *
     * For all albums in a compilation the pair album-title/album-artist should be
     * unique as this pair is used as a key in several places.
     *
     * Albums without an artist are called compilations. Albums without a title but
     * with an artist should contain all singles of the specific artist. There should
     * be one album without title and artist for all the rest.
     */
    class AMAROK_CORE_EXPORT Album : public Base
    {
        public:
            virtual QString prettyName() const;

            /**
             * Whether this album is considered to be a compilation of tracks from various
             * artists.
             */
            virtual bool isCompilation() const = 0;
            /**
             * Whether toggling the compilation status is currenlty supported. Default
             * implementation returns false.
             */
            virtual bool canUpdateCompilation() const { return false; }
            /**
             * Set compilation status. You should check canUpdateCompilation() first.
             */
            virtual void setCompilation( bool isCompilation ) { Q_UNUSED( isCompilation ) }

            /** Returns true if this album has an album artist */
            virtual bool hasAlbumArtist() const = 0;
            /** Returns a pointer to the album's artist */
            virtual ArtistPtr albumArtist() const = 0;
            /** returns all tracks on this album */
            virtual TrackList tracks() = 0;

            /**
             * A note about image sizes:
             *  when size is <= 1, return the full size image
             */
            /** returns true if the album has a cover set */
            virtual bool hasImage( int size = 0 ) const { Q_UNUSED( size ); return false; }

            /** Returns the image for the album, usually the cover image.
                The default implementation returns an null image.
                If you need a pixmap call The::coverCache()->getCover( album, size )
                instead. That function also returns a "nocover" pixmap
            */
            virtual QImage image( int size = 0 ) const;

            /** Returns the image location on disk.
                The mpris interface is using this information for notifications so
                it better is a local file url.
            */
            virtual QUrl imageLocation( int size = 0 ) { Q_UNUSED( size ); return QUrl(); }

            /** Returns true if it is possible to update the cover of the album */
            virtual bool canUpdateImage() const { return false; }

            /** updates the cover of the album
                @param image The large scale image that should be used as cover for the album.
                Note: the parameter should not be a QPixmap as a pixmap can only be created reliable in a UI thread.
            */
            virtual void setImage( const QImage &image ) { Q_UNUSED( image ); }

            /** removes the album art */
            virtual void removeImage() { }

            /** don't automatically fetch artwork */
            virtual void setSuppressImageAutoFetch( const bool suppress ) { Q_UNUSED( suppress ); }
            /** should automatic artwork retrieval be suppressed? */
            virtual bool suppressImageAutoFetch() const { return false; }

            virtual bool operator==( const Meta::Album &album ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_CORE_EXPORT Composer : public Base
    {
        public:
            virtual QString prettyName() const;

            /** returns all tracks by this composer */
            virtual TrackList tracks() = 0;

            virtual bool operator==( const Meta::Composer &composer ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_CORE_EXPORT Genre : public Base
    {
        public:
            virtual QString prettyName() const;

            /** returns all tracks which belong to the genre */
            virtual TrackList tracks() = 0;

            virtual bool operator==( const Meta::Genre &genre ) const;

        protected:
            virtual void notifyObservers() const;
    };

    class AMAROK_CORE_EXPORT Year : public Base
    {
        public:
            /**
             * Returns the year this object represents.
             * number of 0 is considered unset.
             */
            virtual int year() const { return name().toInt(); }

            /** returns all tracks which are tagged with this year */
            virtual TrackList tracks() = 0;

            virtual bool operator==( const Meta::Year &year ) const;

        protected:
            virtual void notifyObservers() const;
    };

    /**
     * A Label represents an arbitrary classification of a Track.
     */
    class AMAROK_CORE_EXPORT Label : public Base
    {
        // we need nothing more than what Meta::Base has
    };
}

Q_DECLARE_METATYPE( Meta::TrackPtr )
Q_DECLARE_METATYPE( Meta::TrackList )
Q_DECLARE_METATYPE( Meta::ArtistPtr )
Q_DECLARE_METATYPE( Meta::ArtistList )
Q_DECLARE_METATYPE( Meta::AlbumPtr )
Q_DECLARE_METATYPE( Meta::AlbumList )
Q_DECLARE_METATYPE( Meta::ComposerPtr )
Q_DECLARE_METATYPE( Meta::ComposerList )
Q_DECLARE_METATYPE( Meta::GenrePtr )
Q_DECLARE_METATYPE( Meta::GenreList )
Q_DECLARE_METATYPE( Meta::YearPtr )
Q_DECLARE_METATYPE( Meta::YearList )
Q_DECLARE_METATYPE( Meta::LabelPtr )
Q_DECLARE_METATYPE( Meta::LabelList )

#endif /* AMAROK_META_H */
