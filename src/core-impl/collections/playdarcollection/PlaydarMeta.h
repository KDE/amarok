/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
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

#ifndef PLAYDAR_META_H
#define PLAYDAR_META_H

#include "core/meta/Meta.h"

#include <QDateTime>
#include <QList>
#include <QString>
#include <QWeakPointer>

#include <KSharedPtr>
#include <QUrl>

namespace Collections
{
    class Collection;
    class PlaydarCollection;
}

namespace Meta
{
    class PlaydarTrack;
    class PlaydarArtist;
    class PlaydarAlbum;
    class PlaydarGenre;
    class PlaydarComposer;
    class PlaydarYear;
    class PlaydarLabel;

    typedef KSharedPtr< PlaydarTrack > PlaydarTrackPtr;
    typedef QList< PlaydarTrackPtr > PlaydarTrackList;
    typedef KSharedPtr< PlaydarArtist > PlaydarArtistPtr;
    typedef QList< PlaydarArtistPtr > PlaydarArtistList;
    typedef KSharedPtr< PlaydarAlbum > PlaydarAlbumPtr;
    typedef QList< PlaydarAlbumPtr > PlaydarAlbumList;
    typedef KSharedPtr< PlaydarComposer > PlaydarComposerPtr;
    typedef QList< PlaydarComposerPtr > PlaydarComposerList;
    typedef KSharedPtr< PlaydarGenre > PlaydarGenrePtr;
    typedef QList< PlaydarGenrePtr > PlaydarGenreList;
    typedef KSharedPtr< PlaydarYear > PlaydarYearPtr;
    typedef QList< PlaydarYearPtr > PlaydarYearList;
    typedef KSharedPtr< PlaydarLabel > PlaydarLabelPtr;
    typedef QList< PlaydarLabelPtr > PlaydarLabelList;
    
    class PlaydarTrack : public Track
    {
        public:
            PlaydarTrack( QString &sid,
                          QString &playableUrl,
                          QString &name,
                          QString &artist,
                          QString &album,
                          QString &mimetype,
                          double score,
                          qint64 length,
                          int bitrate,
                          int filesize,
                          QString &source );
            ~PlaydarTrack();
            
            QString name() const;
            QUrl playableUrl() const;
            QString prettyUrl() const;
            QString uidUrl() const;
            QString sid() const;
            QString notPlayableReason() const;

            AlbumPtr album() const;
            ArtistPtr artist() const;
            ComposerPtr composer() const;
            GenrePtr genre() const;
            YearPtr year() const;
            LabelList labels() const;
            qreal bpm() const;
            QString comment() const;
            double score() const;
            qint64 length() const;
            int filesize() const;
            int sampleRate() const;
            int bitrate() const;
            QDateTime createDate() const;
            int trackNumber() const;
            int discNumber() const;
            
            QString type() const;
            
            bool inCollection() const;
            Collections::Collection* collection() const;
            
            QString cachedLyrics() const;
            void setCachedLyrics( const QString &lyrics );
            
            void addLabel( const QString &label );
            void addLabel( const LabelPtr &label );
            void removeLabel( const LabelPtr &label );

            StatisticsPtr statistics();

            //PlaydarTrack-specific:
            QString source() const;
            QString mimetype() const;
            
            void addToCollection( Collections::PlaydarCollection *collection );
            void setAlbum( PlaydarAlbumPtr album );
            void setArtist( PlaydarArtistPtr artist );
            void setComposer( PlaydarComposerPtr composer );
            void setGenre( PlaydarGenrePtr genre );
            void setYear( PlaydarYearPtr year );
            
            PlaydarAlbumPtr playdarAlbum();
            PlaydarArtistPtr playdarArtist();
            PlaydarComposerPtr playdarComposer();
            PlaydarGenrePtr playdarGenre();
            PlaydarYearPtr playdarYear();
            PlaydarLabelList playdarLabels();

        private:
            QWeakPointer< Collections::PlaydarCollection > m_collection;
            
            PlaydarAlbumPtr m_album;
            PlaydarArtistPtr m_artist;
            PlaydarComposerPtr m_composer;
            PlaydarGenrePtr m_genre;
            PlaydarYearPtr m_year;
            PlaydarLabelList m_labelList;
            Meta::StatisticsPtr m_statsStore;

            QString m_sid;
            QUrl m_uidUrl;
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

            QString m_source;
    };
    
    class PlaydarArtist : public Artist
    {
        public:
            PlaydarArtist( const QString &name );
            ~PlaydarArtist();

            QString name() const;

            TrackList tracks();
            AlbumList albums();

            void addTrack( PlaydarTrackPtr newTrack );
            void addAlbum( PlaydarAlbumPtr newAlbum );
            
        private:
            QString m_name;
            TrackList m_tracks;
            AlbumList m_albums;
    };
    
    class PlaydarAlbum : public Album
    {
        public:
            PlaydarAlbum( const QString &name );
            ~PlaydarAlbum();
            bool isCompilation() const;
            
            QString name() const;
            
            bool hasAlbumArtist() const;
            ArtistPtr albumArtist() const;
            TrackList tracks();
            bool hasImage( int size = 0 ) const;
            QImage image( int size = 0 ) const;
            QUrl imageLocation( int size = 0 );
            bool canUpdateImage() const;
            void setImage( const QImage &image );
            void removeImage();
            void setSuppressImageAutoFetch( const bool suppress );
            bool suppressImageAutoFetch() const;

            void addTrack( PlaydarTrackPtr newTrack );
            void setAlbumArtist( PlaydarArtistPtr newAlbumArtist );

        private:
            QString m_name;
            TrackList m_tracks;
            bool m_isCompilation;
            ArtistPtr m_albumArtist;
            bool m_suppressImageAutoFetch;
            mutable bool m_triedToFetchCover;
            mutable QImage m_cover;
    };

    class PlaydarComposer : public Composer
    {
        public:
            PlaydarComposer( const QString &name );
            ~PlaydarComposer();

            QString name() const;

            TrackList tracks();

            void addTrack( PlaydarTrackPtr newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
    
    class PlaydarGenre : public Genre
    {
        public:
            PlaydarGenre( const QString &name );
            ~PlaydarGenre();

            QString name() const;

            TrackList tracks();

            void addTrack( PlaydarTrackPtr newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
    
    class PlaydarYear : public Year
    {
        public:
            PlaydarYear( const QString &name );
            ~PlaydarYear();

            QString name() const;

            TrackList tracks();

            void addTrack( PlaydarTrackPtr newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
    
    class PlaydarLabel : public Label
    {
        public:
            PlaydarLabel( const QString &name );
            ~PlaydarLabel();

            QString name() const;

            void addTrack( PlaydarTrackPtr newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
}

#endif /* PLAYDARMETA_H */
