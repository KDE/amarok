/* This file is part of the KDE project
   Copyright (C) 2007 Bart Cerneels <bart.cerneels@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef PODCASTMETABASE_H
#define PODCASTMETABASE_H

#include "meta.h"

#include <QStringList>

using namespace Meta;

class PodcastTrack;
class PodcastAlbum;
class PodcastArtist;
class PodcastGenre;
class PodcastComposer;
class PodcastYear;

typedef KSharedPtr<PodcastTrack> PodcastTrackPtr;
typedef KSharedPtr<PodcastArtist> PodcastArtistPtr;
typedef KSharedPtr<PodcastAlbum> PodcastAlbumPtr;
typedef KSharedPtr<PodcastGenre> PodcastGenrePtr;
typedef KSharedPtr<PodcastComposer> PodcastComposerPtr;
typedef KSharedPtr<PodcastYear> PodcastYearPtr;


typedef QList<PodcastTrackPtr > PodcastTrackList;
typedef QList<PodcastArtistPtr > PodcastArtistList;
typedef QList<PodcastAlbumPtr > PodcastAlbumList;
typedef QList<PodcastComposerPtr> PodcastComposerList;
typedef QList<PodcastGenrePtr > PodcastGenreList;
typedef QList<PodcastYearPtr > PodcastYearList;

// class PodcastMetaFactory
// {
//
//     public:
//         PodcastMetaFactory( const QString &dbPrefix );
//         virtual ~PodcastMetaFactory() {};
//
//         QString tablePrefix();
//
//         virtual TrackPtr createTrack( const QStringList &rows );
//
//         virtual AlbumPtr createAlbum( const QStringList &rows );
//
//         virtual ArtistPtr createArtist( const QStringList &rows );
//
//         virtual GenrePtr createGenre( const QStringList &rows );
//
//     private:
//
//         QString m_dbTablePrefix;
//
//
// };

class PodcastMetaCommon
{
    public:
//         PodcastMetaCommon();
        virtual ~PodcastMetaCommon() { }

        virtual QString title() const = 0;
        virtual QString description() const = 0;

        virtual void setTitle( const QString &title ) = 0;
        virtual void setDescription( const QString &description ) = 0;
};

class PodcastTrack : public Meta::Track, public PodcastMetaCommon
{
    public:
        //Give this a displayable name as some services has terrible names for their streams
        PodcastTrack();

        virtual ~PodcastTrack();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual KUrl playableUrl() const;
        virtual QString url() const;
        virtual QString prettyUrl() const;

        virtual bool isPlayable() const;
        virtual bool isEditable() const;

        virtual AlbumPtr album() const;
        virtual ArtistPtr artist() const;
        virtual GenrePtr genre() const;
        virtual ComposerPtr composer() const;
        virtual YearPtr year() const;

        virtual void setAlbum ( const QString &newAlbum );
        virtual void setArtist ( const QString &newArtist );
        virtual void setGenre ( const QString &newGenre );
        virtual void setComposer ( const QString &newComposer );
        virtual void setYear ( const QString &newYear );

        virtual QString comment() const;
        virtual void setComment ( const QString &newComment );

        virtual double score() const;
        virtual void setScore ( double newScore );

        virtual int rating() const;
        virtual void setRating ( int newRating );

        virtual int length() const;

        virtual int filesize() const;
        virtual int sampleRate() const;
        virtual int bitrate() const;

        virtual int trackNumber() const;
        virtual void setTrackNumber ( int newTrackNumber );

        virtual int discNumber() const;
        virtual void setDiscNumber ( int newDiscNumber );

        virtual uint lastPlayed() const;
        virtual int playCount() const;

        virtual QString type() const;

        virtual void beginMetaDataUpdate() {}    //read only
        virtual void endMetaDataUpdate() {}      //read only
        virtual void abortMetaDataUpdate() {}    //read only

        virtual QString title() const { return m_name; }
        virtual void setTitle( const QString &newTitle ) { m_name = newTitle; }
        virtual QString description() const { return m_description; }
        virtual void setDescription( const QString &description ) { m_description = description; }

//         virtual void processInfoOf( InfoParserBase * infoParser );

        //PodcastTrack specific methods

        void setAlbum( AlbumPtr album );
        void setArtist( ArtistPtr artist );
        void setComposer( ComposerPtr composer );
        void setGenre( GenrePtr genre );
        void setYear( YearPtr year );

        void setLength( int length );

        void setId( int id );
        int id( ) const;
        void setAlbumId( int albumId );
        int albumId() const;
        void setAlbumName( const QString &name );
        QString albumName() const;
        void setArtistId( int id );
        int artistId() const;
        void setArtistName( const QString &name );
        QString artistName() const;
        void setUrl( const QString &url );

    private:
        ArtistPtr m_artist;
        AlbumPtr m_album;
        GenrePtr m_genre;
        ComposerPtr m_composer;
        YearPtr m_year;

        int m_id;
        QString m_name;
        int m_trackNumber;
        int m_length;
        QString m_displayUrl;
        QString m_playableUrl;
        int m_albumId;
        QString m_albumName;
        int m_artistId;
        QString m_artistName;

        QString m_type;
        QString m_description;
};

class PodcastArtist : public Meta::Artist
{
    public:

        PodcastArtist( );
        PodcastArtist( const QString & name );
        virtual ~PodcastArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

//         virtual void processInfoOf( InfoParserBase * infoParser );

        //PodcastArtist specific methods

        void addTrack( TrackPtr track );

        void setDescription( const QString &description );
        QString description( ) const;
        void setId( int id );
        int id( ) const;
        void setTitle( const QString &title );

    private:
        int m_id;
        QString m_name;
        QString m_description;
        TrackList m_tracks;

};

class PodcastAlbum : public Meta::Album, public PodcastMetaCommon
{
    public:
        PodcastAlbum();
        PodcastAlbum( const QString & name  );
        virtual ~PodcastAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual void image() const;
        virtual bool canUpdateImage() const;
        virtual void updateImage();

        virtual QString title() const { return m_name; }
        virtual void setTitle( const QString &newTitle ) { m_name = newTitle; }
        virtual QString description() const { return m_description; }
        virtual void setDescription( const QString &description ) { m_description = description; }

//         virtual void processInfoOf( InfoParserBase * infoParser );




        //PodcastAlbum specific methods

        void addTrack( TrackPtr track );
        void setAlbumArtist( ArtistPtr artist );
        void setIsCompilation( bool compilation );

        void setId( int id );
        int id( ) const;
        void setArtistId( int artistId );
        int artistId( ) const;
        void setArtistName( const QString &name );
        QString artistName() const;

    private:
        int m_id;
        QString m_name;
        TrackList m_tracks;
        bool m_isCompilation;
        ArtistPtr m_albumArtist;
        QString m_description;
        int m_artistId;
        QString m_artistName;
};

class PodcastGenre : public Meta::Genre
{
    public:
        PodcastGenre( const QString &name );
        PodcastGenre( const QStringList &row );
        virtual ~PodcastGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

//         virtual void processInfoOf( InfoParserBase * infoParser );

        //PodcastGenre specific methods
        void addTrack( PodcastTrackPtr track );
        void setName( const QString &name );
        int albumId();
        void setAlbumId( int albumId );

    private:
        int m_albumId;
        QString m_name;
        TrackList m_tracks;
};

class PodcastComposer : public Meta::Composer
{
    public:
        PodcastComposer( const QString &name );
        virtual ~PodcastComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

//         virtual void processInfoOf( InfoParserBase * infoParser );

        //PodcastComposer specific methods
        void addTrack( PodcastTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class PodcastYear : public Meta::Year
{
    public:
        PodcastYear( const QString &name );
        virtual ~PodcastYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

//         virtual void processInfoOf( InfoParserBase * infoParser );

        //PodcastYear specific methods
        void addTrack( PodcastTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

#endif
