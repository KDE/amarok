/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "servicemodelitembase.h"

#include "debug.h"

/*QString ServiceModelItemBase::getUrl() {
    return QString();
} 

ServiceModelItemBase *ServiceModelItemBase::child(int row) {
    return m_childItems.value(row);
}

int ServiceModelItemBase::childCount() const {
     return m_childItems.count();
}
*/

ServiceModelItemBase::~ServiceModelItemBase()
{
    qDeleteAll(m_childItems);
}

ServiceModelItemBase *ServiceModelItemBase::parent() {
    return m_parent;
}

KUrl::List ServiceModelItemBase::getUrls()
{

    KUrl::List urls;

    debug() << "own url: " << getUrl() << endl;
    if ( !getUrl().isEmpty() )
        urls += KUrl( getUrl() );
    
    prePopulate();
    populate();

    foreach( ServiceModelItemBase * childItem, m_childItems ) {
        urls += childItem->getUrls();
    }

    return urls;
}

bool ServiceModelItemBase::operator<( const ServiceModelItemBase& other ) const {
    debug() << "here!!" << endl;
    return data(0).toString() < other.data(0).toString();
}


/*QList<ServiceModelItemBase*> ServiceModelItemBase::getChildItems() const {
    return m_childItems;
}


bool ServiceModelItemBase::hasChildren () const {
    return !m_childItems.isEmpty();
}


int ServiceModelItemBase::columnCount() const {
    return 1;
}*/
