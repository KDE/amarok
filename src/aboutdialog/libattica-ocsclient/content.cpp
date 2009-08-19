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

using namespace Attica;

Content::Content()
  : m_rating( 0 ), m_downloads( 0 )
{
}

void Content::setId( const QString &u )
{
  m_id = u;
}

QString Content::id() const
{
  return m_id;
}

void Content::setName( const QString &name )
{
  m_name = name;
}

QString Content::name() const
{
  return m_name;
}
  
void Content::setRating( int v )
{
  m_rating = v;
}

int Content::rating() const
{
  return m_rating;
}
    
void Content::setDownloads( int v )
{
  m_downloads = v;
}

int Content::downloads() const
{
  return m_downloads;
}
    
void Content::setCreated( const QDateTime &d )
{
  m_created = d;
}

QDateTime Content::created() const
{
  return m_created;
}

void Content::setUpdated( const QDateTime &d )
{
  m_updated = d;
}

QDateTime Content::updated() const
{
  return m_updated;
}

void Content::addExtendedAttribute( const QString &key, const QString &value )
{
  m_extendedAttributes.insert( key, value );
}

QString Content::extendedAttribute( const QString &key ) const
{
  return m_extendedAttributes.value( key );
}

QMap<QString,QString> Content::extendedAttributes() const
{
  return m_extendedAttributes;
}
