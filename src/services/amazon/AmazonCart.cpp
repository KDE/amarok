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

#include "AmazonCart.h"

#include "Amazon.h"
#include "AmazonConfig.h"

#include <QDebug>

AmazonCart* AmazonCart::m_instance = 0;

AmazonCart* AmazonCart::instance()
{
    if( !m_instance )
       m_instance = new AmazonCart();

    return m_instance;
}

void AmazonCart::destroy()
{
    if( m_instance )
    {
        delete m_instance;
        m_instance = 0;
    }
}

AmazonCart::~AmazonCart()
{
}

AmazonCart::AmazonCart()
{
    m_price = 0;
}

void AmazonCart::add( QString asin, QString price, QString name )
{
    AmazonCartItem item( asin, price, name );
    m_price = m_price + price.toInt();
    insert( size(), item );
}

void AmazonCart::clear()
{
    QList<AmazonCartItem>::clear();
    m_price = 0;
}

QStringList AmazonCart::stringList()
{
    QStringList result;

    for( int i = 0; i < size(); i++ )
    {
        result.append( at( i ).prettyName() + " (" + Amazon::prettyPrice( at( i ).price() ) + ')' );
    }

    return result;
}

QString AmazonCart::price()
{
    QString price;
    return price.setNum( m_price );
}

void AmazonCart::remove( int pos )
{
    if( pos < 0 || pos >= size()  ) // not valid
        return;

    m_price = m_price - at( pos ).price().toInt();
    QList<AmazonCartItem>::removeAt( pos );
}

QUrl AmazonCart::checkoutUrl()
{
    if( isEmpty() ) // we don't create empty carts
        return QUrl();

    QString url;

    // the basics
    url += "http://www.mp3-music-store.de/index.php?apikey=";
    url += MP3_MUSIC_STORE_KEY;
    url += "&method=CreateCart&Location=";
    url += AmazonConfig::instance()->country();
    url += "&Player=amarok";

    // let's add the ASINs
    for( int i = 0; i < size(); i++ )
    {
        url += "&ASINs[]=";
        url += at( i ).asin();
    }

    // http://www.mp3-music-store.de/index.php?apikey=27274503cb405cb1929f353fc507f09c&method=CreateCart&Location=de&Player=amarok&ASINs[]=B003MJQB9A&ASINs[]=B0049B5GKU

    return QUrl( url );
}
