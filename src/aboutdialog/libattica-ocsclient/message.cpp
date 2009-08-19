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

#include "message.h"

using namespace Attica;

Message::Message()
  : m_status( Unread )
{
}

void Message::setId( const QString &u )
{
  m_id = u;
}

QString Message::id() const
{
  return m_id;
}

void Message::setFrom( const QString &n )
{
  m_from = n;
}

QString Message::from() const
{
  return m_from;
}
  
void Message::setTo( const QString &n )
{
  m_to = n;
}

QString Message::to() const
{
  return m_to;
}
    
void Message::setSent( const QDateTime &d )
{
  m_sent = d;
}

QDateTime Message::sent() const
{
  return m_sent;
}

void Message::setStatus( Message::Status s )
{
  m_status = s;
}

Message::Status Message::status() const
{
  return m_status;
}

void Message::setStatusText( const QString &c )
{
  m_statusText = c;
}

QString Message::statusText() const
{
  return m_statusText;
}

void Message::setSubject( const QString &subject )
{
  m_subject = subject;
}

QString Message::subject() const
{
  return m_subject;
}

void Message::setBody( const QString &body )
{
  m_body = body;
}

QString Message::body() const
{
  return m_body;
}
