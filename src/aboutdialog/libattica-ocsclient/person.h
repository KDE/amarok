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
#ifndef ATTICA_PERSON_H
#define ATTICA_PERSON_H

#include <QtCore/QDate>
#include <QtCore/QList>
#include <QtGui/QPixmap>

#include <KUrl>

#include "atticaclient_export.h"


namespace Attica {

class ATTICA_EXPORT Person
{
  public:
    typedef QList<Person> List;
  
    Person();

    void setId( const QString & );
    QString id() const;

    void setFirstName( const QString & );
    QString firstName() const;
    
    void setLastName( const QString & );
    QString lastName() const;
    
    void setBirthday( const QDate & );
    QDate birthday() const;

    void setCountry( const QString & );
    QString country() const;

    void setLatitude( qreal );
    qreal latitude() const;

    void setLongitude( qreal );
    qreal longitude() const;

    void setAvatarUrl( const KUrl & );
    KUrl avatarUrl() const;

    void setAvatar( const QPixmap & );
    QPixmap avatar() const;

    void setHomepage( const QString & );
    QString homepage() const;

    void setCity( const QString & );
    QString city() const;

    void addExtendedAttribute( const QString &key, const QString &value );
    QString extendedAttribute( const QString &key ) const;

    QMap<QString,QString> extendedAttributes() const;

  private:
    QString m_id;  
    QString m_firstName;
    QString m_lastName;
    QDate m_birthday;
    QString m_country;
    qreal m_latitude;
    qreal m_longitude;
    KUrl m_avatarUrl;
    QPixmap m_avatar;
    QString m_homepage;
    QString m_city;

    QMap<QString,QString> m_extendedAttributes;
};

}

#endif
