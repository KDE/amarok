/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
 * The Amazon store in based upon the Magnatune store in Amarok,                        *
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#ifndef AMAZONMETA_H
#define AMAZONMETA_H

#include "../ServiceMetaBase.h"
#include "../ServiceAlbumCoverDownloader.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <QtGlobal>

class AmazonStore;

namespace Meta
{

///////////////////////////////////////////////////////////////////////////////
// class AmazonItem
///////////////////////////////////////////////////////////////////////////////

/* Amazon items contain all the Amazon specific stuff and are the base class for everything that
   can be added to the amazon shopping cart.
 * ASIN: Amazon Standard Identification Number, see
   https://secure.wikimedia.org/wikipedia/en/wiki/Amazon_Standard_Identification_Number
 * Price: price of the item, in cents (or whatever the smallest unit of the currency is called)
 */

class AmazonItem : public QObject
{
    Q_OBJECT

public:
    virtual void setAsin( const QString asin );
    virtual QString asin() const;

    virtual void setPrice( const QString price );
    virtual QString price() const;

    virtual bool isBookmarkable() const { return true; }
    virtual QString collectionName() const { return "Amazon"; }

private:
    QAction* m_addToCartAction;
    QString m_asin;
    QString m_price;
};

///////////////////////////////////////////////////////////////////////////////
// class AmazonAlbum
///////////////////////////////////////////////////////////////////////////////

class AmazonAlbum : public ServiceAlbumWithCover, public AmazonItem
{
public:
    AmazonAlbum( const QStringList & resultRow );

    virtual void setCoverUrl( const QString &coverUrl );
    virtual QString coverUrl() const;

    virtual QString downloadPrefix() const { return "amazon"; }

    virtual QUrl imageLocation( int size = 1 ) { Q_UNUSED( size ); return QUrl( coverUrl() ); }

    virtual bool isBookmarkable() const { return true; }
    virtual QString collectionName() const { return "Amazon"; }
    virtual bool simpleFiltering() const { return false; }

private:
    QString m_coverUrl;
};

///////////////////////////////////////////////////////////////////////////////
// class AmazonArtist
///////////////////////////////////////////////////////////////////////////////

class AmazonArtist : public ServiceArtist
{
public:
    AmazonArtist( const QStringList & resultRow );

    virtual bool isBookmarkable() const { return true; }
    virtual QString collectionName() const { return "Amazon"; }
    virtual bool simpleFiltering() const { return false; }
};


///////////////////////////////////////////////////////////////////////////////
// class AmazonTrack
///////////////////////////////////////////////////////////////////////////////

class AmazonTrack : public ServiceTrack, public AmazonItem
{
public:
    AmazonTrack( const QStringList & resultRow );

    virtual QPixmap emblem();
    virtual QString sourceDescription();
    virtual QString sourceName();

    virtual bool isBookmarkable() const { return true; }
    virtual QString collectionName() const { return "Amazon"; }
    virtual bool simpleFiltering() const { return false; }
};

} // namespace Meta

///////////////////////////////////////////////////////////////////////////////
// class AmazonMetaFactory
///////////////////////////////////////////////////////////////////////////////

class AmazonMetaFactory : public ServiceMetaFactory
{
    public:
        AmazonMetaFactory( const QString &dbPrefix );
        virtual ~AmazonMetaFactory() {}

        virtual Meta::TrackPtr createTrack( const QStringList &rows );
        virtual Meta::AlbumPtr createAlbum( const QStringList &rows );
        virtual Meta::ArtistPtr createArtist( const QStringList &rows );
};

#endif // AMAZONMETA_H
