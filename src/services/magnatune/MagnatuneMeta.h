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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MAGNATUNEMETA_H
#define MAGNATUNEMETA_H


#include "../ServiceMetaBase.h"
#include "../ServiceAlbumCoverDownloader.h"
#include "../ShowInServiceAction.h"

#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

class MagnatuneAlbumCoverDownloader;
class MagnatuneStore;

namespace Meta
{
    
class MagnatuneTrack  : public ServiceTrack
{
    Q_OBJECT
public:
    MagnatuneTrack( const QString &name );
    MagnatuneTrack( const QStringList &resultRow );

    QString lofiUrl();
    void setLofiUrl( const QString &url );

    QList<QString> moods();
    void setMoods(  QList<QString> moods );

    void setDownloadMembership();

    virtual QList< QAction *> customActions();
    virtual QList< QAction *> currentTrackActions();

    virtual QString sourceName();
    virtual QString sourceDescription();
    virtual QPixmap emblem();

    virtual bool isBookmarkable() { return true; }
    virtual QString collectionName() { return "Magnatune.com"; }
    virtual bool simpleFiltering() { return false; }

    void setOggUrl( const QString& url );
    QString oggUrl() const;

    void setAlbumPtr( Meta::AlbumPtr album );

public slots:
    void purchase();

private:
    QString m_lofiUrl;
    QString m_oggUrl;
    bool m_downloadMembership;
    QList<QString> m_moods;
    QAction * m_purchaseAction;
    ShowInServiceAction * m_showInServiceAction;

};


class MagnatuneArtist : public ServiceArtist
{
private:

    QString m_photoUrl;
    QString m_magnatuneUrl;

public:
    MagnatuneArtist( const QString &name );
    MagnatuneArtist( const QStringList &resultRow );

    void setPhotoUrl( const QString &photoUrl );
    QString photoUrl() const;

    void setMagnatuneUrl( const QString &url );
    QString magnatuneUrl() const;

    virtual bool isBookmarkable() { return true; }
    virtual QString collectionName() { return "Magnatune.com"; }
    virtual bool simpleFiltering() { return false; }
};

class MagnatuneAlbum : public ServiceAlbumWithCover
{
    Q_OBJECT
private:
    QString m_coverUrl;
    int m_launchYear;
    QString m_albumCode;
    MagnatuneStore * m_store;
    bool m_downloadMembership;


public:
    MagnatuneAlbum( const QString &name );
    MagnatuneAlbum( const QStringList &resultRow );

    ~MagnatuneAlbum();
    
    virtual QString downloadPrefix() const { return "magnatune"; }

    void setLaunchYear( int launchYear );
    int launchYear() const;

    virtual void setCoverUrl( const QString &coverUrl );
    virtual QString coverUrl() const;
    
    void setAlbumCode(  const QString &albumCode );
    QString albumCode();

    virtual QList< QAction *> customActions();

    void setStore( MagnatuneStore * store );
    MagnatuneStore * store();

    void setDownloadMembership();

    virtual bool isBookmarkable() { return true; }
    virtual QString collectionName() { return "Magnatune.com"; }
    virtual bool simpleFiltering() { return false; }

public slots:
    void purchase();
    void addToFavorites();
private:
    QAction * m_purchaseAction;
    QAction * m_addToFavoritesAction;
};

class MagnatuneGenre  : public ServiceGenre
{

public:
    MagnatuneGenre( const QString &name );
    MagnatuneGenre( const QStringList &resultRow );

    virtual bool isBookmarkable() { return true; }
    virtual QString collectionName() { return "Magnatune.com"; }
    virtual bool simpleFiltering() { return false; }
};

}

class MagnatuneMetaFactory : public ServiceMetaFactory
{

    public:
        enum { OGG = 0, MP3 = 1, LOFI = 2 };
        
        MagnatuneMetaFactory( const QString &dbPrefix, MagnatuneStore * store );
        virtual ~MagnatuneMetaFactory() {}

        virtual int getTrackSqlRowCount();
        virtual QString getTrackSqlRows();
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

        //stuff for supporting the new membership services at Magnatune.com

        void setMembershipInfo ( const QString &prefix, const QString &userName, const QString &password );
        void setStreamType( int type );

    private:
        QString m_membershipPrefix;
        int m_streamType;

        QString m_userName;
        QString m_password;
        MagnatuneStore * m_store;
};


#endif
