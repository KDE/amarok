/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef MAGNATUNEMETA_H
#define MAGNATUNEMETA_H


#include "../ServiceMetaBase.h"
#include "../ServiceAlbumCoverDownloader.h"

#include <QList>
#include <QString>
#include <QStringList>

class MagnatuneStore;

namespace Meta
{
    
class MagnatuneTrack  : public ServiceTrack
{
    Q_OBJECT
public:
    explicit MagnatuneTrack( const QString &name );
    explicit  MagnatuneTrack( const QStringList &resultRow );

    QString lofiUrl();
    void setLofiUrl( const QString &url );

    QList<QString> moods() const;
    void setMoods(  const QList<QString> &moods );

    void setDownloadMembership();

    QString sourceName() override;
    QString sourceDescription() override;
    QPixmap emblem() override;

    bool isBookmarkable() const override { return true; }
    QString collectionName() const override { return QStringLiteral("Magnatune.com"); }
    bool simpleFiltering() const override { return false; }

    void setOggUrl( const QString& url );
    QString oggUrl() const;

    void setAlbumPtr( const Meta::AlbumPtr &album ) override;

public Q_SLOTS:
    void download();

private:
    QString m_lofiUrl;
    QString m_oggUrl;
    bool m_downloadMembership;
    QList<QString> m_moods;
};


class MagnatuneArtist : public ServiceArtist
{
private:

    QUrl m_photoUrl;
    QUrl m_magnatuneUrl;

public:
    explicit MagnatuneArtist( const QString &name );
    explicit MagnatuneArtist( const QStringList &resultRow );

    void setPhotoUrl( const QUrl &photoUrl );
    QUrl photoUrl() const;

    void setMagnatuneUrl( const QUrl &url );
    QUrl magnatuneUrl() const;

    bool isBookmarkable() const override { return true; }
    QString collectionName() const override { return QStringLiteral("Magnatune.com"); }
    bool simpleFiltering() const override { return false; }
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
    explicit MagnatuneAlbum( const QString &name );
    explicit MagnatuneAlbum( const QStringList &resultRow );

    ~MagnatuneAlbum() override;
    
    QString downloadPrefix() const override { return QStringLiteral("magnatune"); }

    void setLaunchYear( int launchYear );
    int launchYear() const;

    void setCoverUrl( const QString &coverUrl ) override;
    QString coverUrl() const override;

    QUrl imageLocation( int size = 1 ) override { Q_UNUSED( size ); return QUrl( coverUrl() ); }
    
    void setAlbumCode(  const QString &albumCode );
    QString albumCode();

    void setStore( MagnatuneStore * store );
    MagnatuneStore * store();

    void setDownloadMembership();

    bool isBookmarkable() const override { return true; }
    QString collectionName() const override { return QStringLiteral("Magnatune.com"); }
    bool simpleFiltering() const override { return false; }

public Q_SLOTS:
    void download();
    void addToFavorites();
};

class MagnatuneGenre  : public ServiceGenre
{

public:
    explicit MagnatuneGenre( const QString &name );
    explicit MagnatuneGenre( const QStringList &resultRow );

    bool isBookmarkable() const override { return true; }
    QString collectionName() const override { return QStringLiteral("Magnatune.com"); }
    bool simpleFiltering() const override { return false; }
};

}

class MagnatuneMetaFactory : public ServiceMetaFactory
{

    public:
        enum { OGG = 0, MP3 = 1, LOFI = 2 };
        
        MagnatuneMetaFactory( const QString &dbPrefix, MagnatuneStore * store );
        ~MagnatuneMetaFactory() override {}

        int getTrackSqlRowCount() override;
        QString getTrackSqlRows() override;
        Meta::TrackPtr createTrack( const QStringList &rows ) override;

        int getAlbumSqlRowCount() override;
        QString getAlbumSqlRows() override;
        Meta::AlbumPtr createAlbum( const QStringList &rows ) override;

        int getArtistSqlRowCount() override;
        QString getArtistSqlRows() override;
        Meta::ArtistPtr createArtist( const QStringList &rows ) override;

    //virtual int getGenreSqlRowCount();
    //virtual QString getGenreSqlRows();
        Meta::GenrePtr createGenre( const QStringList &rows ) override;

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
