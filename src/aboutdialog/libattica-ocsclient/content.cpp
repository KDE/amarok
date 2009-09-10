/*
    This file is part of KDE.

    Copyright (c) 2008 Cornelius Schumacher <schumacher@kde.org>

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

#include "content.h"

#include <QtCore/QDateTime>


using namespace Attica;

class Content::Private : public QSharedData {
    public:
        QDateTime m_created;
        int m_downloads;
        QString m_id;  
        QString m_name;
        int m_rating;
        QDateTime m_updated;

        QMap<QString,QString> m_extendedAttributes;

        Private()
            : m_downloads(0),
              m_rating(0)
        {
        }
};


Content::Content()
  : d(new Private)
{
}

Content::Content(const Attica::Content& other)
    : d(other.d)
{
}

Content& Content::operator=(const Attica::Content & other)
{
    d = other.d;
    return *this;
}

Content::~Content()
{
}


void Content::setId( const QString &u )
{
  d->m_id = u;
}

QString Content::id() const
{
  return d->m_id;
}

void Content::setName( const QString &name )
{
  d->m_name = name;
}

QString Content::name() const
{
  return d->m_name;
}
  
void Content::setRating( int v )
{
  d->m_rating = v;
}

int Content::rating() const
{
  return d->m_rating;
}
    
void Content::setDownloads( int v )
{
  d->m_downloads = v;
}

int Content::downloads() const
{
  return d->m_downloads;
}
    
void Content::setCreated( const QDateTime &date )
{
  d->m_created = date;
}

QDateTime Content::created() const
{
  return d->m_created;
}

void Content::setUpdated( const QDateTime &date )
{
  d->m_updated = date;
}

QDateTime Content::updated() const
{
  return d->m_updated;
}

void Content::addExtendedAttribute( const QString &key, const QString &value )
{
  d->m_extendedAttributes.insert( key, value );
}

QString Content::extendedAttribute( const QString &key ) const
{
  return d->m_extendedAttributes.value( key );
}

QMap<QString,QString> Content::extendedAttributes() const
{
  return d->m_extendedAttributes;
}
