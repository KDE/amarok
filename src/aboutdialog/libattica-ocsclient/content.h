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
#ifndef ATTICA_CONTENT_H
#define ATTICA_CONTENT_H

#include "atticaclient_export.h"

#include <QtCore>

namespace Attica {

class ATTICA_EXPORT Content
{
  public:
    typedef QList<Content> List;
  
    Content();

    void setId( const QString & );
    QString id() const;

    void setName( const QString & );
    QString name() const;

    void setRating( int );
    int rating() const;
    
    void setDownloads( int );
    int downloads() const;
    
    void setCreated( const QDateTime & );
    QDateTime created() const;

    void setUpdated( const QDateTime & );
    QDateTime updated() const;

    void addExtendedAttribute( const QString &key, const QString &value );
    QString extendedAttribute( const QString &key ) const;

    QMap<QString,QString> extendedAttributes() const;

  private:
    QString m_id;  
    QString m_name;
    int m_rating;
    int m_downloads;
    QDateTime m_created;
    QDateTime m_updated;

    QMap<QString,QString> m_extendedAttributes;
};

}

#endif
