/* This file is part of the KDE project
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

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

#ifndef SERVICEMETABASE_H
#define SERVICEMETABASE_H

#include "meta.h"

#include <QStringList>

using namespace Meta;

class ServiceTrack;
class ServiceAlbum;
class ServiceArtist;
class ServiceGenre;
class ServiceComposer;
class ServiceYear;

typedef KSharedPtr<ServiceTrack> ServiceTrackPtr;
typedef KSharedPtr<ServiceArtist> ServiceArtistPtr;
typedef KSharedPtr<ServiceAlbum> ServiceAlbumPtr;
typedef KSharedPtr<ServiceGenre> ServiceGenrePtr;
typedef KSharedPtr<ServiceComposer> ServiceComposerPtr;
typedef KSharedPtr<ServiceYear> ServiceYearPtr;


typedef QList<ServiceTrackPtr > ServiceTrackList;
typedef QList<ServiceArtistPtr > ServiceArtistList;
typedef QList<ServiceAlbumPtr > ServiceAlbumList;
typedef QList<ServiceComposerPtr> ServiceComposerList;
typedef QList<ServiceGenrePtr > ServiceGenreList;
typedef QList<ServiceYearPtr > ServiceYearList;

class ServiceMetaFactory
{

public:
    ServiceMetaFactory( const QString &dbPrefix );
    virtual ~ServiceMetaFactory() {};

    QString tablePrefix();

    virtual int getTrackSqlRowCount();
    virtual QString getTrackSqlRows();
    virtual TrackPtr createTrack( const QStringList &rows );

    virtual int getAlbumSqlRowCount();
    virtual QString getAlbumSqlRows();
    virtual AlbumPtr createAlbum( const QStringList &rows );

    virtual int getArtistSqlRowCount();
    virtual QString getArtistSqlRows();
    virtual ArtistPtr createArtist( const QStringList &rows );

    virtual int getGenreSqlRowCount();
    virtual QString getGenretSqlRows();
    virtual GenrePtr createGenre( const QStringList &rows );

private:

    QString m_dbTablePrefix;


};

class ServiceTrack : public Meta::Track
{
    public:
        //Give this a displayable name as some services has terrible names for their streams
        ServiceTrack( const QString & name );

        //create track based on an sql query result
        ServiceTrack( const QStringList & resultRow );
        virtual ~ServiceTrack();

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

        virtual void subscribe ( TrackObserver *observer );
        virtual void unsubscribe ( TrackObserver *observer );

        //ServiceTrack specific methods

        void setAlbum( AlbumPtr album );
        void setArtist( ArtistPtr artist ); 
        void setComposer( ComposerPtr composer );
        void setGenre( GenrePtr genre );
        void setYear( YearPtr year );

        void setTitle( const QString &newTitle );
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
};

class ServiceArtist : public Meta::Artist
{
    public:

        ServiceArtist( const QStringList & resultRow );
        ServiceArtist( const QString & name );
        virtual ~ServiceArtist();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //ServiceArtist specific methods

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

class ServiceAlbum : public Meta::Album
{
    public:
        ServiceAlbum( const QStringList & resultRow );
        ServiceAlbum( const QString & name  );
        virtual ~ServiceAlbum();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual bool isCompilation() const;
        virtual bool hasAlbumArtist() const;
        virtual ArtistPtr albumArtist() const;
        virtual TrackList tracks();

        virtual void image() const;
        virtual bool canUpdateImage() const;
        virtual void updateImage();


        

        //ServiceAlbum specific methods

        void addTrack( TrackPtr track );
        void setAlbumArtist( ArtistPtr artist );
        void setIsCompilation( bool compilation );

        void setDescription( const QString &description );
        QString description( ) const;
        void setId( int id );
        int id( ) const;
        void setArtistId( int artistId );
        int artistId( ) const;
        void setArtistName( const QString &name );
        QString artistName() const;
        void setTitle( const QString &title );

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

class ServiceGenre : public Meta::Genre
{
    public:
        ServiceGenre( const QString &name );
        ServiceGenre( const QStringList &row );
        virtual ~ServiceGenre();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //ServiceGenre specific methods
        void addTrack( ServiceTrackPtr track );
        void setName( const QString &name );
        int trackId();
        void setTrackId( int trackId ); 

    private:
        int m_trackId;
        QString m_name;
        TrackList m_tracks;
};

class ServiceComposer : public Meta::Composer
{
    public:
        ServiceComposer( const QString &name );
        virtual ~ServiceComposer();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //ServiceComposer specific methods
        void addTrack( ServiceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

class ServiceYear : public Meta::Year
{
    public:
        ServiceYear( const QString &name );
        virtual ~ServiceYear();

        virtual QString name() const;
        virtual QString prettyName() const;

        virtual TrackList tracks();

        //ServiceYear specific methods
        void addTrack( ServiceTrackPtr track );

    private:
        QString m_name;
        TrackList m_tracks;
};

#endif

