/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef JAMENDOMETA_H
#define JAMENDOMETA_H


#include "../ServiceMetaBase.h"
#include "../ServiceAlbumCoverDownloader.h"
#include "../ShowInServiceAction.h"

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QList>

class JamendoService;

class JamendoMetaFactory : public ServiceMetaFactory
{

public:
    JamendoMetaFactory( const QString &dbPrefix, JamendoService * service );
    virtual ~JamendoMetaFactory() {}

    //virtual int getTrackSqlRowCount();
    //virtual QString getTrackSqlRows();
    virtual Meta::TrackPtr createTrack( const QStringList &rows );

    virtual int getAlbumSqlRowCount();
    virtual QString getAlbumSqlRows();
    virtual Meta::AlbumPtr createAlbum( const QStringList &rows );

    virtual int getArtistSqlRowCount();
    virtual QString getArtistSqlRows();
    virtual Meta::ArtistPtr createArtist( const QStringList &rows );

    //virtual int getGenreSqlRowCount();
    //virtual QString getGenreSqlRows();
    virtual Meta::GenrePtr createGenre( const QStringList &rows );

private:

    JamendoService * m_service;

};


namespace Meta
{

class JamendoTrack  : public ServiceTrack
{


public:
    JamendoTrack( const QString &name );
    JamendoTrack( const QStringList &resultRow );

    void setService( JamendoService * service );

    virtual QString sourceName();
    virtual QString sourceDescription();
    virtual QPixmap emblem();
    virtual QString scalableEmblem();

   /**
    * Get the file type.
    * Since jamendo uses interesting redirects, we cannot use the base implementation
    * which relies on getting the file type from the url.
    */
    virtual QString type() const;

    virtual bool isBookmarkable() { return true; }
    virtual QString collectionName() { return "Jamendo.com"; }
    virtual bool simpleFiltering() { return false; }

    virtual QList< QAction *> customActions();
    virtual QList< QAction * > currentTrackActions();


private:
    
    JamendoService * m_service;
    
    QAction * m_downloadCustomAction;
    QAction * m_downloadCurrentTrackAction;
    ShowInServiceAction * m_showInServiceAction;

};


class JamendoArtist : public ServiceArtist
{

private:

    QString m_country;
    QString m_photoURL;
    QString m_jamendoURL;
    QString m_homeURL;

public:
    JamendoArtist( const QString &name );
    JamendoArtist( const QStringList &resultRow );

    void setPhotoURL( const QString &photoURL );
    QString photoURL() const;

    void setCountry( const QString &country );
    QString country() const;

    void setHomeURL( const QString &homeURL );
    QString homeURL() const;

    void setJamendoURL( const QString &jamendoURL );
    QString jamendoURL() const;

    virtual bool isBookmarkable() { return true; }
    virtual QString collectionName() { return "Jamendo.com"; }
    virtual bool simpleFiltering() { return false; }
};


class JamendoAlbum  : public ServiceAlbumWithCover
{
private:
    float m_popularity;
    QString m_coverURL;
    int m_launchYear;
    QString m_genre;
    QString m_mp3TorrentUrl;
    QString m_oggTorrentUrl;


public:
    JamendoAlbum( const QString &name );
    JamendoAlbum( const QStringList &resultRow );

        
    virtual QString downloadPrefix() const { return "jamendo"; }
    
    void setPopularity( float popularity );
    float popularity() const;

    virtual void setCoverUrl( const QString &coverURL );
    virtual QString coverUrl() const;

    void setLaunchYear( int launchYear );
    int launchYear() const;

    void setGenre( const QString &genre );
    QString genre() const;

    void setMp3TorrentUrl( const QString &url );
    QString mp3TorrentUrl();

    void setOggTorrentUrl( const QString &url );
    QString oggTorrentUrl();

    virtual QList< QAction *> customActions();

    void setService( JamendoService * store );
    JamendoService * service();

    virtual bool isBookmarkable() { return true; }
    virtual QString collectionName() { return "Jamendo.com"; }
    virtual bool simpleFiltering() { return false; }

private:
    JamendoService * m_service;


};

class JamendoGenre  : public ServiceGenre
{

public:
    JamendoGenre( const QString &name );
    JamendoGenre( const QStringList &resultRow );

    virtual bool isBookmarkable() { return true; }
    virtual QString collectionName() { return "Jamendo.com"; }
    virtual bool simpleFiltering() { return false; }

};

}

#endif
