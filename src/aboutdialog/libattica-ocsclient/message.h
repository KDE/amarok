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
#ifndef ATTICA_MESSAGE_H
#define ATTICA_MESSAGE_H

#include <QtCore/QDateTime>
#include <QtCore/QList>

#include "atticaclient_export.h"


namespace Attica {

class ATTICA_EXPORT Message
{
  public:
    typedef QList<Message> List;

    enum Status { Unread = 0, Read = 1 };
  
    Message();

    void setId( const QString & );
    QString id() const;

    void setFrom( const QString & );
    QString from() const;
    
    void setTo( const QString & );
    QString to() const;
    
    void setSent( const QDateTime & );
    QDateTime sent() const;

    void setStatus( Status );
    Status status() const;

    void setStatusText( const QString & );
    QString statusText() const;

    void setSubject( const QString & );
    QString subject() const;

    void setBody( const QString & );
    QString body() const;

  private:
    QString m_id;
    QString m_from;
    QString m_to;
    QDateTime m_sent;
    Status m_status;
    QString m_statusText;
    QString m_subject;
    QString m_body;
};

}

#endif
