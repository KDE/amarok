/***************************************************************************
 *   Copyright (C) 2004 by Michael Schulze                                 *
 *   mike.s@genion.de                                                      *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "listitem.h"

namespace itunesdb {

ListItem::ListItem(int type)
{
    itemType = type;
}

ListItem::ListItem() {
    itemType = ITEMTYPE_NONE;
}

ListItem::~ListItem()
{
}

int ListItem::getType() const {
    return itemType;
}

void ListItem::setItemProperty(const QString& data, ItemProperty field) {
    if( !data.isEmpty())
        properties[ field]= data;
}

const QString& ListItem::getItemProperty( ItemProperty field) const {
    return properties[ field];
}

int ListItem::getNumComponents() const {
    return properties.count();
}

void ListItem::doneAddingData() {
    // This method may be overridden for consistency checks after all properties have been set.
    // Does nothing here.
}

}
