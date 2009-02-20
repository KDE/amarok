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

#include "SmartPointerList.h"

#include "Debug.h"


SmartPointerList::SmartPointerList( QObject* parent )
    : QObject( parent )
{
    DEBUG_BLOCK
}

SmartPointerList::~SmartPointerList()
{
    DEBUG_BLOCK
}


void
SmartPointerList::addPointer( QObject* pointer )
{
    DEBUG_BLOCK

    debug() << "Adding Pointer: " << pointer;
    debug() << "Current size of list: " << size();
    
    connect( pointer, SIGNAL( destroyed( QObject* ) ), SLOT( removePointer( QObject* ) ) );
    append( pointer );
}

void
SmartPointerList::removePointer( QObject* pointer ) // SLOT
{
    DEBUG_BLOCK

    debug() << "Current size of list: " << size();

    removeAll( pointer );
}


#include "SmartPointerList.moc"

