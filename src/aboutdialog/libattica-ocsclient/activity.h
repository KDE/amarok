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
#ifndef ATTICA_ACTIVITY_H
#define ATTICA_ACTIVITY_H

#include "atticaclient_export.h"

#include <kurl.h>

#include <QtCore>

namespace Attica {


class ATTICA_EXPORT Activity
{
  public:
    typedef QList<Activity> List;

    Activity();

    void setId( const QString & );
    QString id() const;

    void setUser( const QString & );
    QString user() const;

    void setTimestamp( const QDateTime & );
    QDateTime timestamp() const;

    void setMessage( const QString & );
    QString message() const;

    void setLink( const QString & );
    QString link() const;

  private:
    QString m_id;
    QString m_user;
    QDateTime m_timestamp;
    QString m_message;
    QString m_link;
};

}

#endif
