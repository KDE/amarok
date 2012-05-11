/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

#include "AmazonShoppingCartItem.h"

AmazonShoppingCartItem::AmazonShoppingCartItem( QString asin, QString price, QString prettyName )
{
    m_asin = asin;
    m_prettyName = prettyName;
    m_price = price;
}

QString
AmazonShoppingCartItem::asin() const
{
    return m_asin;
}

QString
AmazonShoppingCartItem::prettyName() const
{
    return m_prettyName;
}

QString
AmazonShoppingCartItem::price() const
{
    return m_price;
}
