/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

#include "AmazonActions.h"
#include "AmazonCart.h"
#include "AmazonMeta.h"

#include <KStandardDirs>
#include "klocalizedstring.h"

using namespace Meta;

///////////////////////////////////////////////////////////////////////////////
// class AmazonAlbum
///////////////////////////////////////////////////////////////////////////////

/*
    resultRow[0]: id
    resultRow[1]: name
    resultRow[2]: description
    resultRow[3]: artistId
    resultRow[4]: price
    resultRow[5]: coverUrl
    resultRow[6]: ASIN
*/
Meta::AmazonAlbum::AmazonAlbum( const QStringList & resultRow )
    : ServiceAlbumWithCover( resultRow )
{
    setPrice( resultRow[4] );
    setCoverUrl( resultRow[5] );
    setAsin( resultRow[6] );
}

void Meta::AmazonAlbum::setCoverUrl( const QString & coverUrl )
{
    m_coverUrl = coverUrl;
}

QString Meta::AmazonAlbum::coverUrl() const
{
    return m_coverUrl;
}


///////////////////////////////////////////////////////////////////////////////
// class AmazonArtist
///////////////////////////////////////////////////////////////////////////////

/*
    resultRow[0]: id
    resultRow[1]: name
    resultRow[2]: description
*/

Meta::AmazonArtist::AmazonArtist( const QStringList & resultRow )
    : ServiceArtist( resultRow )
{
}


///////////////////////////////////////////////////////////////////////////////
// class AmazonItem
///////////////////////////////////////////////////////////////////////////////

void Meta::AmazonItem::addToCart()
{
    //AmazonCart::instance()->add( m_asin, m_price );
}

QList< QAction * > Meta::AmazonItem::customActions()
{
    DEBUG_BLOCK
    QList< QAction * > actions;

    if ( !m_addToCartAction ) {
        QString actionLabel = i18n( "Amazon: &Add item to cart" );
        m_addToCartAction = new AmazonAddToCartAction( actionLabel, this );
    }

    actions.append( m_addToCartAction );

    return actions;
}

void Meta::AmazonItem::setAsin( QString asin )
{
    m_asin = asin;
}

QString Meta::AmazonItem::asin() const
{
    return m_asin;
}

void Meta::AmazonItem::setPrice( const QString price )
{
    m_price = price;
}

QString Meta::AmazonItem::price() const
{
    return m_price;
}


///////////////////////////////////////////////////////////////////////////////
// class AmazonTrack
///////////////////////////////////////////////////////////////////////////////

/*
    resultRow[0]: id
    resultRow[1]: name
    resultRow[2]: trackNumber
    resultRow[3]: length
    resultRow[4]: playableUrl
    resultRow[5]: albumId
    resultRow[6]: artistId
    resultRow[7]: price
    resultRow[8]: asin
*/

Meta::AmazonTrack::AmazonTrack( const QStringList & resultRow )
    : ServiceTrack( resultRow )
{
    setPrice( resultRow[7] );
    setAsin( resultRow[8] );
}

QPixmap Meta::AmazonTrack::emblem()
{
    return QPixmap( KStandardDirs::locate( "data", "amarok/images/emblem-amazon.png" ) );
}

void Meta::AmazonTrack::setAlbumPtr( Meta::AlbumPtr album )
{
    ServiceTrack::setAlbumPtr( album );
}

QString Meta::AmazonTrack::sourceDescription()
{
    return i18n( "Snippet taken from the Amazon MP3 store" );
}

QString Meta::AmazonTrack::sourceName()
{
    return "Amazon";
}


///////////////////////////////////////////////////////////////////////////////
// class AmazonMetaFactory
///////////////////////////////////////////////////////////////////////////////

AmazonMetaFactory::AmazonMetaFactory( const QString &dbPrefix, AmazonStore* store )
    : ServiceMetaFactory( dbPrefix )
    , m_store( store )
{}


TrackPtr AmazonMetaFactory::createTrack( const QStringList &rows )
{
    AmazonTrack* track = new AmazonTrack( rows );

    //track->setUidUrl( url );

    return TrackPtr( track );
}


AlbumPtr AmazonMetaFactory::createAlbum( const QStringList &rows )
{
    AmazonAlbum* album = new AmazonAlbum( rows );

    album->setSourceName( "Amazon" );

    return AlbumPtr( album );
}


ArtistPtr AmazonMetaFactory::createArtist( const QStringList &rows )
{
    AmazonArtist* artist = new AmazonArtist( rows );
    artist->setSourceName( "Amazon" );

    return ArtistPtr( artist );
}

