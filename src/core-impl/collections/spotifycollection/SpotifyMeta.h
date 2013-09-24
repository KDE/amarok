/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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
#ifndef SPOTIFY_META_H
#define SPOTIFY_META_H

#include "core/meta/Meta.h"

namespace Collections
{
    class Collection;
    class SpotifyCollection;
}

namespace Meta
{
    class SpotifyAlbum;
    class SpotifyArtist;
    class SpotifyComposer;
    class SpotifyGenre;
    class SpotifyLabel;
    class SpotifyTrack;
    class SpotifyYear;

    typedef KSharedPtr< SpotifyAlbum > SpotifyAlbumPtr;
    typedef KSharedPtr< SpotifyArtist > SpotifyArtistPtr;
    typedef KSharedPtr< SpotifyComposer > SpotifyComposerPtr;
    typedef KSharedPtr< SpotifyGenre > SpotifyGenrePtr;
    typedef KSharedPtr< SpotifyLabel > SpotifyLabelPtr;
    typedef KSharedPtr< SpotifyTrack > SpotifyTrackPtr;
    typedef KSharedPtr< SpotifyYear > SpotifyYearPtr;

    typedef QList< SpotifyAlbumPtr > SpotifyAlbumList;
    typedef QList< SpotifyArtistPtr > SpotifyArtistList;
    typedef QList< SpotifyComposerPtr > SpotifyComposerList;
    typedef QList< SpotifyGenrePtr > SpotifyGenreList;
    typedef QList< SpotifyLabelPtr > SpotifyLabelList;
    typedef QList< SpotifyTrackPtr > SpotifyTrackList;
    typedef QList< SpotifyYearPtr > SpotifyYearList;

    class SpotifyTrack : public Track
    {
        public:
            SpotifyTrack( const QString &playableUrl,
                          const QString &name,
                          const QString &artist,
                          const QString &album,
                          const int     year,
                          const int     trackNumber,
                          const int     discNumber,
                          const QString &genre,
                          const QString &mimetype,
                          const double score,
                          const qint64 length,
                          const int bitrate,
                          const int filesize, const QString &source );
            ~SpotifyTrack();

            QString name() const;
            KUrl playableUrl() const;
            QString prettyUrl() const;
            QString uidUrl() const;

            bool isPlayable() const;
            AlbumPtr album() const;
            ArtistPtr artist() const;
            ComposerPtr composer() const;
            GenrePtr genre() const;
            YearPtr year() const;
            LabelList labels() const;
            qreal bpm() const;
            QString comment() const;
            double score() const;
            void setScore( double newScore );
            int rating() const;
            void setRating( int newRating );
            qint64 length() const;
            int filesize() const;
            int sampleRate() const;
            int bitrate() const;
            QDateTime createDate() const;
            int trackNumber() const;
            int discNumber() const;
            int playCount() const;

            QString type() const;
            QString notPlayableReason() const;

            void prepareToPlay();

            void finishedPlaying( double playedFraction );

            bool inCollection() const;
            Collections::Collection* collection() const;

            QString cachedLyrics() const;
            void setCachedLyrics( const QString &lyrics );

            void addLabel( const QString &label );
            void addLabel( const LabelPtr &label );
            void removeLabel( const LabelPtr &label );

            //SpotifyTrack-specific:
            QString source() const;
            QString mimetype() const;

            void addToCollection( Collections::SpotifyCollection *collection );
            void setAlbum( SpotifyAlbumPtr album );
            void setArtist( SpotifyArtistPtr artist );
            void setComposer( SpotifyComposerPtr composer );
            void setGenre( SpotifyGenrePtr genre );
            void setYear( SpotifyYearPtr year );

            SpotifyAlbumPtr spotifyAlbum();
            SpotifyArtistPtr spotifyArtist();
            SpotifyComposerPtr spotifyComposer();
            SpotifyGenrePtr spotifyGenre();
            SpotifyYearPtr spotifyYear();
            SpotifyLabelList spotifyLabels();

        private:
            QWeakPointer< Collections::SpotifyCollection > m_collection;

            SpotifyAlbumPtr m_album;
            SpotifyArtistPtr m_artist;
            SpotifyComposerPtr m_composer;
            SpotifyGenrePtr m_genre;
            SpotifyYearPtr m_year;
            SpotifyLabelList m_labelList;

            KUrl m_uidUrl;
            QString m_playableUrl;
            QString m_name;
            QString m_mimetype;
            double m_score;
            qint64 m_length;
            int m_bitrate;
            int m_filesize;
            int m_trackNumber;
            int m_discNumber;
            QDateTime m_createDate;
            QString m_comment;
            int m_rating;
            int m_playcount;

            QString m_source;
    };

    class SpotifyArtist : public Artist
    {
        public:
            SpotifyArtist( const QString &name );
            ~SpotifyArtist();

            QString name() const;

            TrackList tracks();
            AlbumList albums();

            void addTrack( SpotifyTrackPtr newTrack );
            void addAlbum( SpotifyAlbumPtr newAlbum );

        private:
            QString m_name;
            TrackList m_tracks;
            AlbumList m_albums;
    };

    class SpotifyAlbum : public Album
    {
        public:
            SpotifyAlbum( const QString &name );
            ~SpotifyAlbum();
            bool isCompilation() const;

            QString name() const;

            bool hasAlbumArtist() const;
            ArtistPtr albumArtist() const;
            TrackList tracks();
            bool hasImage( int size = 0 ) const;
            QImage image( int size = 0 ) const;
            KUrl imageLocation( int size = 0 );
            bool canUpdateImage() const;
            void setImage( const QImage &image );
            void removeImage();
            void setSuppressImageAutoFetch( const bool suppress );
            bool suppressImageAutoFetch() const;

            void addTrack( SpotifyTrackPtr newTrack );
            void setAlbumArtist( SpotifyArtistPtr newAlbumArtist );

        private:
            QString m_name;
            TrackList m_tracks;
            bool m_isCompilation;
            ArtistPtr m_albumArtist;
            bool m_suppressImageAutoFetch;
            mutable bool m_triedToFetchCover;
            mutable QImage m_cover;
    };

    class SpotifyComposer : public Composer
    {
        public:
            SpotifyComposer( const QString &name );
            ~SpotifyComposer();

            QString name() const;

            TrackList tracks();

            void addTrack( SpotifyTrackPtr newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };

    class SpotifyGenre : public Genre
    {
        public:
            SpotifyGenre( const QString &name );
            ~SpotifyGenre();

            QString name() const;

            TrackList tracks();

            void addTrack( SpotifyTrackPtr newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };

    class SpotifyYear : public Year
    {
        public:
            SpotifyYear( const QString &name );
            ~SpotifyYear();

            QString name() const;

            TrackList tracks();

            void addTrack( SpotifyTrackPtr newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };

    class SpotifyLabel : public Label
    {
        public:
            SpotifyLabel( const QString &name );
            ~SpotifyLabel();

            QString name() const;

            void addTrack( SpotifyTrackPtr newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
}

//Q_DECLARE_METATYPE( Meta::SpotifyTrack )
//Q_DECLARE_METATYPE( Meta::SpotifyArtist )
//Q_DECLARE_METATYPE( Meta::SpotifyAlbum )
//Q_DECLARE_METATYPE( Meta::SpotifyComposer )
//Q_DECLARE_METATYPE( Meta::SpotifyGenre )
//Q_DECLARE_METATYPE( Meta::SpotifyYear )
//Q_DECLARE_METATYPE( Meta::SpotifyLabel )

#endif
