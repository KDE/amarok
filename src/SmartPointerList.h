/***************************************************************************
 *   Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef AMAROK_SMARTPOINTERLIST_H
#define AMAROK_SMARTPOINTERLIST_H 

#include <QList>    // baseclass
#include <QObject>  // baseclass

/**
    A QList for storing pointers to QObjects, that automatically removes the pointers when objects are deleted.
    @author Mark Kretschmann <kretschmann@kde.org> 
*/

class SmartPointerList : public QObject, public QList<QObject*>
{
    Q_OBJECT

public:
    SmartPointerList( QObject* parent = 0 );
    ~SmartPointerList();

    void addPointer( QObject* pointer );

protected Q_SLOTS:
    void removePointer( QObject* pointer ); 
};


#endif
