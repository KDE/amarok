/*
    This file is part of KDE.

    Copyright (c) 2009 Eckhart Wörner <ewoerner@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#include "event.h"


using namespace AmarokAttica;


Event::Event()
    : m_latitude(0), m_longitude(0)
{
}


void Event::setId(const QString& id)
{
    m_id = id;
}

QString Event::id() const
{
    return m_id;
}


void Event::setName(const QString& name)
{
    m_name = name;
}

QString Event::name() const
{
    return m_name;
}


void Event::setDescription(const QString& text)
{
    m_description = text;
}

QString Event::description() const
{
    return m_description;
}


void Event::setUser(const QString& id)
{
    m_user = id;
}

QString Event::user() const
{
    return m_user;
}


void Event::setStartDate(const QDate& date)
{
    m_startDate = date;
}

QDate Event::startDate() const
{
    return m_startDate;
}


void Event::setEndDate(const QDate& date)
{
    m_endDate = date;
}

QDate Event::endDate() const
{
    return m_endDate;
}


void Event::setLatitude(qreal lat)
{
    m_latitude = lat;
}

qreal Event::latitude() const
{
    return m_latitude;
}


void Event::setLongitude(qreal lon)
{
    m_longitude = lon;
}

qreal Event::longitude() const
{
    return m_longitude;
}


void Event::setHomepage(const QString& url)
{
    m_homepage = url;
}

QString Event::homepage() const
{
    return m_homepage;
}


void Event::setCountry(const QString& country)
{
    m_country = country;
}

QString Event::country() const
{
    return m_country;
}


void Event::setCity(const QString& city)
{
    m_city = city;
}

QString Event::city() const
{
    return m_city;
}


void Event::addExtendedAttribute(const QString& key, const QString& value)
{
    m_extendedAttributes.insert(key, value);
}

QString Event::extendedAttribute(const QString& key) const
{
    return m_extendedAttributes.value(key);
}

QMap<QString, QString> Event::extendedAttributes() const
{
    return m_extendedAttributes;
}
