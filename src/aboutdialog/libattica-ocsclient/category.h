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
#ifndef ATTICA_CATEGORY_H
#define ATTICA_CATEGORY_H

#include <QtCore/QSharedDataPointer>
#include <QtCore/QList>

#include "atticaclient_export.h"


namespace Attica {


class ATTICA_EXPORT Category
{
  public:
    typedef QList<Category> List;
  
    Category();
    Category(const Category& other);
    Category& operator=(const Category& other);
    ~Category();

    void setId( const QString & );
    QString id() const;

    void setName( const QString & );
    QString name() const;

  private:
    class Private;
    QSharedDataPointer<Private> d;
};

}

#endif
