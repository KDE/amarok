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
#include <QPointer>

#include "AmarokSharedPointer.h"
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

    typedef AmarokSharedPointer< PlaydarTrack > PlaydarTrackPtr;
    typedef QList< PlaydarTrackPtr > PlaydarTrackList;
    typedef AmarokSharedPointer< PlaydarArtist > PlaydarArtistPtr;
    typedef QList< PlaydarArtistPtr > PlaydarArtistList;
    typedef AmarokSharedPointer< PlaydarAlbum > PlaydarAlbumPtr;
    typedef QList< PlaydarAlbumPtr > PlaydarAlbumList;
    typedef AmarokSharedPointer< PlaydarComposer > PlaydarComposerPtr;
    typedef QList< PlaydarComposerPtr > PlaydarComposerList;
    typedef AmarokSharedPointer< PlaydarGenre > PlaydarGenrePtr;
    typedef QList< PlaydarGenrePtr > PlaydarGenreList;
    typedef AmarokSharedPointer< PlaydarYear > PlaydarYearPtr;
    typedef QList< PlaydarYearPtr > PlaydarYearList;
    typedef AmarokSharedPointer< PlaydarLabel > PlaydarLabelPtr;
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
            ~PlaydarTrack() override;
            
            QString name() const override;
            QUrl playableUrl() const override;
            QString prettyUrl() const override;
            QString uidUrl() const override;
            QString sid() const;
            QString notPlayableReason() const override;

            AlbumPtr album() const override;
            ArtistPtr artist() const override;
            ComposerPtr composer() const override;
            GenrePtr genre() const override;
            YearPtr year() const override;
            LabelList labels() const override;
            qreal bpm() const override;
            QString comment() const override;
            double score() const;
            qint64 length() const override;
            int filesize() const override;
            int sampleRate() const override;
            int bitrate() const override;
            QDateTime createDate() const override;
            int trackNumber() const override;
            int discNumber() const override;
            
            QString type() const override;
            
            bool inCollection() const override;
            Collections::Collection* collection() const override;
            
            QString cachedLyrics() const override;
            void setCachedLyrics( const QString &lyrics ) override;
            
            void addLabel( const QString &label ) override;
            void addLabel( const LabelPtr &label ) override;
            void removeLabel( const LabelPtr &label ) override;

            StatisticsPtr statistics() override;

            //PlaydarTrack-specific:
            QString source() const;
            QString mimetype() const;
            
            void addToCollection( Collections::PlaydarCollection *collection );
            void setAlbum( const PlaydarAlbumPtr &album );
            void setArtist( const PlaydarArtistPtr &artist );
            void setComposer( const PlaydarComposerPtr &composer );
            void setGenre( const PlaydarGenrePtr &genre );
            void setYear( const PlaydarYearPtr &year );
            
            PlaydarAlbumPtr playdarAlbum();
            PlaydarArtistPtr playdarArtist();
            PlaydarComposerPtr playdarComposer();
            PlaydarGenrePtr playdarGenre();
            PlaydarYearPtr playdarYear();
            PlaydarLabelList playdarLabels();

        private:
            QPointer< Collections::PlaydarCollection > m_collection;
            
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
            explicit PlaydarArtist( const QString &name );
            ~PlaydarArtist() override;

            QString name() const override;

            TrackList tracks() override;
            AlbumList albums();

            void addTrack( const PlaydarTrackPtr &newTrack );
            void addAlbum( const PlaydarAlbumPtr &newAlbum );
            
        private:
            QString m_name;
            TrackList m_tracks;
            AlbumList m_albums;
    };
    
    class PlaydarAlbum : public Album
    {
        public:
            explicit PlaydarAlbum( const QString &name );
            ~PlaydarAlbum() override;
            bool isCompilation() const override;
            
            QString name() const override;
            
            bool hasAlbumArtist() const override;
            ArtistPtr albumArtist() const override;
            TrackList tracks() override;
            bool hasImage( int size = 0 ) const override;
            QImage image( int size = 0 ) const override;
            QUrl imageLocation( int size = 0 ) const override;
            bool canUpdateImage() const override;
            void setImage( const QImage &image ) override;
            void removeImage() override;
            void setSuppressImageAutoFetch( const bool suppress ) override;
            bool suppressImageAutoFetch() const override;

            void addTrack( const PlaydarTrackPtr &newTrack );
            void setAlbumArtist( const PlaydarArtistPtr &newAlbumArtist );

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
            explicit PlaydarComposer( const QString &name );
            ~PlaydarComposer() override;

            QString name() const override;

            TrackList tracks() override;

            void addTrack( const PlaydarTrackPtr &newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
    
    class PlaydarGenre : public Genre
    {
        public:
            explicit PlaydarGenre( const QString &name );
            ~PlaydarGenre() override;

            QString name() const override;

            TrackList tracks() override;

            void addTrack( const PlaydarTrackPtr &newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
    
    class PlaydarYear : public Year
    {
        public:
            explicit PlaydarYear( const QString &name );
            ~PlaydarYear() override;

            QString name() const override;

            TrackList tracks() override;

            void addTrack( const PlaydarTrackPtr &newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
    
    class PlaydarLabel : public Label
    {
        public:
            explicit PlaydarLabel( const QString &name );
            ~PlaydarLabel() override;

            QString name() const override;

            void addTrack( const PlaydarTrackPtr &newTrack );

        private:
            QString m_name;
            TrackList m_tracks;
    };
}

#endif /* PLAYDARMETA_H */
