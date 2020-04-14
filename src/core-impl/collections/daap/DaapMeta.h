/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef DAAPMETA_H
#define DAAPMETA_H

#include "core/meta/Meta.h"

namespace Collections {
    class DaapCollection;
}

namespace Meta
{

class DaapTrack;
class DaapAlbum;
class DaapArtist;
class DaapGenre;
class DaapComposer;
class DaapYear;

typedef AmarokSharedPointer<DaapTrack> DaapTrackPtr;
typedef AmarokSharedPointer<DaapArtist> DaapArtistPtr;
typedef AmarokSharedPointer<DaapAlbum> DaapAlbumPtr;
typedef AmarokSharedPointer<DaapGenre> DaapGenrePtr;
typedef AmarokSharedPointer<DaapComposer> DaapComposerPtr;
typedef AmarokSharedPointer<DaapYear> DaapYearPtr;

class DaapTrack : public Meta::Track
{
    public:
        DaapTrack( Collections::DaapCollection *collection, const QString &host, quint16 port, const QString &dbId, const QString &itemId, const QString &format);
        ~DaapTrack() override;

        QString name() const override;

        QUrl playableUrl() const override;
        QString uidUrl() const override;
        QString prettyUrl() const override;
        QString notPlayableReason() const override;

        AlbumPtr album() const override;
        ArtistPtr artist() const override;
        GenrePtr genre() const override;
        ComposerPtr composer() const override;
        YearPtr year() const override;

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( int newYear );

        virtual void setTitle( const QString &newTitle );

        qreal bpm() const override;

        QString comment() const override;
        virtual void setComment ( const QString &newComment );

        qint64 length() const override;

        int filesize() const override;
        int sampleRate() const override;
        int bitrate() const override;

        int trackNumber() const override;
        virtual void setTrackNumber ( int newTrackNumber );

        int discNumber() const override;
        virtual void setDiscNumber ( int newDiscNumber );

        QString type() const override;

        bool inCollection() const override;
        Collections::Collection* collection() const override;

        //DaapTrack specific methods
        void setAlbum( const DaapAlbumPtr &album );
        void setArtist( const DaapArtistPtr &artist );
        void setComposer( const DaapComposerPtr &composer );
        void setGenre( const DaapGenrePtr &genre );
        void setYear( const DaapYearPtr &year );

        void setLength( qint64 length );

    private:
        Collections::DaapCollection *m_collection;

        DaapArtistPtr m_artist;
        DaapAlbumPtr m_album;
        DaapGenrePtr m_genre;
        DaapComposerPtr m_composer;
        DaapYearPtr m_year;

        QString m_name;
        QString m_type;
        qint64 m_length;
        int m_trackNumber;
        QString m_displayUrl;
        QString m_playableUrl;
};

class DaapArtist : public Meta::Artist
{
    public:
        explicit DaapArtist( const QString &name );
        ~DaapArtist() override;

        QString name() const override;

        TrackList tracks() override;

        virtual AlbumList albums();

        //DaapArtist specific methods
        void addTrack( const DaapTrackPtr &track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class DaapAlbum : public Meta::Album
{
    public:
        explicit DaapAlbum( const QString &name );
        ~DaapAlbum() override;

        QString name() const override;

        bool isCompilation() const override;
        bool hasAlbumArtist() const override;
        ArtistPtr albumArtist() const override;
        TrackList tracks() override;

        //DaapAlbum specific methods
        void addTrack( const DaapTrackPtr &track );
        void setAlbumArtist( const DaapArtistPtr &artist );

    private:
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        DaapArtistPtr m_albumArtist;
};

class DaapGenre : public Meta::Genre
{
    public:
        explicit DaapGenre( const QString &name );
        ~DaapGenre() override;

        QString name() const override;

        TrackList tracks() override;

        //DaapGenre specific methods
        void addTrack( const DaapTrackPtr &track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class DaapComposer : public Meta::Composer
{
    public:
        explicit DaapComposer( const QString &name );
        ~DaapComposer() override;

        QString name() const override;

        TrackList tracks() override;

        //DaapComposer specific methods
        void addTrack( const DaapTrackPtr &track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class DaapYear : public Meta::Year
{
    public:
        explicit DaapYear( const QString &name );
        ~DaapYear() override;

        QString name() const override;

        TrackList tracks() override;

        //DaapYear specific methods
        void addTrack( const DaapTrackPtr &track );

    private:
        QString m_name;
        TrackList m_tracks;
};

}

#endif

