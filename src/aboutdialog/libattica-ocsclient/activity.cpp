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

#include "activity.h"

#include "activitylistjob.h"

using namespace Attica;

Activity::Activity()
{
}

void Activity::setId( const QString &id )
{
    m_id = id;
}

QString Activity::id() const
{
    return m_id;
}

void Activity::setUser( const QString &u )
{
  m_user = u;
}

QString Activity::user() const
{
  return m_user;
}

void Activity::setTimestamp( const QDateTime &d )
{
  m_timestamp = d;
}

QDateTime Activity::timestamp() const
{
  return m_timestamp;
}

void Activity::setMessage( const QString &c )
{
  m_message = c;
}

QString Activity::message() const
{
  return m_message;
}

void Activity::setLink( const QString &v )
{
  m_link = v;
}

QString Activity::link() const
{
  return m_link;
}
