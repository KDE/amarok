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

#ifndef AMAZONCART_H
#define AMAZONCART_H

#include "AmazonCartItem.h"

#include <QtGlobal>

#include <QString>
#include <QStringList>
#include <QUrl>


/* Singleton representing the Amazon shopping cart. */

class AmazonCart : public QList<AmazonCartItem>
{
public:
    static AmazonCart* instance();
    static void destroy();

    void add( QString asin, QString price, QString name );
    void clear();
    QStringList stringList();
    QString price();
    void remove( int pos );
    QUrl checkoutUrl();

private:
    AmazonCart();
    ~AmazonCart();

    static AmazonCart* m_instance;
    quint64 m_price;
};

#endif // AMAZONCART_H
