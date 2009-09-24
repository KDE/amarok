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

#include "category.h"

#include <QtCore/QString>


using namespace Attica;

class Category::Private : public QSharedData {
    public:
        QString m_id;
        QString m_name;
};


Category::Category() : d(new Private)
{
}

Category::Category(const Attica::Category& other)
    : d(other.d)
{
}

Category& Category::operator=(const Attica::Category & other)
{
    d = other.d;
    return *this;
}

Category::~Category()
{
}


void Category::setId( const QString &u )
{
  d->m_id = u;
}

QString Category::id() const
{
  return d->m_id;
}

void Category::setName( const QString &name )
{
  d->m_name = name;
}

QString Category::name() const
{
  return d->m_name;
}
