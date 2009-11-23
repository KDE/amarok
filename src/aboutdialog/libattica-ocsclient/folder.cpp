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

#include "folder.h"

using namespace AmarokAttica;

Folder::Folder()
  : m_messageCount( 0 )
{
}

void Folder::setId( const QString &u )
{
  m_id = u;
}

QString Folder::id() const
{
  return m_id;
}

void Folder::setName( const QString &d )
{
  m_name = d;
}

QString Folder::name() const
{
  return m_name;
}

void Folder::setMessageCount( int c )
{
  m_messageCount = c;
}

int Folder::messageCount() const
{
  return m_messageCount;
}

void Folder::setType( const QString &v )
{
  m_type = v;
}

QString Folder::type() const
{
  return m_type;
}
