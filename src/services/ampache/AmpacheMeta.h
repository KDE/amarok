/****************************************************************************************
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *           (c) 2013 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef AMPACHEMETA_H
#define AMPACHEMETA_H

#include "ServiceBase.h"
#include "ServiceMetaBase.h"
#include "ServiceAlbumCoverDownloader.h"

#include <QDateTime>
#include <QList>
#include <QSet>
#include <QStandardPaths>
#include <QString>
#include <QStringList>


namespace Meta
{

class AmpacheTrack  : public ServiceTrack
{

public:
    explicit AmpacheTrack( const QString& title, ServiceBase * service = nullptr )
        : ServiceTrack( title )
        , m_service( service )
        , m_discNumber( 0 )
    {
        Q_UNUSED(m_service); // might be used again later
    }

    QString sourceName() override { return QStringLiteral("Ampache"); }
    QString sourceDescription() override { return QStringLiteral("The Ampache music server project: http://Ampache.org"); }
    QPixmap emblem()  override { return QPixmap( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/emblem-ampache.png") ) );  }
    QString scalableEmblem()  override { return  QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/images/emblem-ampache-scalable.svgz") );  }
    QString notPlayableReason() const override;

    int discNumber() const override { return m_discNumber; }
    void setDiscNumber( int newDiscNumber ) override { m_discNumber = newDiscNumber; }

private:
    ServiceBase * m_service;

    int m_discNumber;
};


class AmpacheAlbum  : public ServiceAlbumWithCover
{
private:
    QString m_coverURL;

public:
    explicit AmpacheAlbum( const QString &name );
    explicit AmpacheAlbum( const QStringList &resultRow );

    ~AmpacheAlbum() override;

    QString downloadPrefix() const override { return QStringLiteral("ampache"); }

    void setCoverUrl( const QString &coverURL ) override;
    QString coverUrl() const override;

    bool operator==( const Meta::Album &other ) const override
    {
        return name() == other.name();
    }

    QList<int> ids() const { return m_ampacheAlbums.keys(); }

    struct AmpacheAlbumInfo {
        int id;
        int discNumber;
        int year;
    };

    /** Add an ampache album to this Amarok album.
        Warning: The album will not be automatically
        registered with the new id, same as with setId
    */
    void addInfo( const AmpacheAlbumInfo &info );

    /** Get's an album info for a specific ID */
    AmpacheAlbumInfo getInfo( int id ) const;

private:

    // the unique album key of ampache contains discNumber and year
    // the Amarok key only name and artist
    // so this AmpacheAlbum object represents a number of ampache albums.
    QHash<int, AmpacheAlbumInfo> m_ampacheAlbums;
};

class AmpacheArtist : public ServiceArtist
{
    private:
        QString m_coverURL;

    public:
        AmpacheArtist( const QString &name, ServiceBase * service )
            : ServiceArtist( name )
            , m_service( service )
        {}

        bool isBookmarkable() const override { return true; }
        QString collectionName() const override { return m_service->name(); }
        bool simpleFiltering() const override { return true; }

        bool operator==( const Meta::Artist &other ) const override
        {
            return name() == other.name();
        }

    private:
        ServiceBase * m_service;
};

}

#endif  // End include guard
