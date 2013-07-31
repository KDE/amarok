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

#ifndef AMAZONSHOPPINGCART_H
#define AMAZONSHOPPINGCART_H

#include "AmazonShoppingCartItem.h"

#include <QtGlobal>

#include <QString>
#include <QStringList>
#include <QUrl>


/* Singleton representing the Amazon shopping cart. */

class AmazonShoppingCart : public QList<AmazonShoppingCartItem>
{
public:
    static AmazonShoppingCart* instance();
    static void destroy();

    /**
    * Adds an item to the cart.
    */
    void add( QString asin, QString price, QString name );

    /**
    * Empties the cart.
    */
    void clear();

    /**
    * Returns the list of items in the cart.
    */
    QStringList stringList();

    /**
    * Returns the total price of all items in the cart.
    */
    QString price();

    /**
    * Removes an item from the cart.
    * @param pos position of the item.
    */
    void remove( int pos );

    /**
    * Returns the URL required to check the items in the cart out.
    ** @asin single item to check out (optional).
    */
    QUrl checkoutUrl( QString asin = QString() );

private:
    AmazonShoppingCart();
    ~AmazonShoppingCart();

    static AmazonShoppingCart* m_instance;
    quint64 m_price;
};

#endif // AMAZONSHOPPINGCART_H
