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

#ifndef AMAROKSERVICEMODELITEMBASE_H
#define AMAROKSERVICEMODELITEMBASE_H 

#include "kurl.h"

#include <QString>
#include <QVariant>

class ServiceModelItemBase
{
public:

    virtual ~ServiceModelItemBase();

    virtual QVariant data(int column) const = 0;

    virtual ServiceModelItemBase *child(int row) = 0;
    virtual int childCount() const = 0;
    virtual int columnCount() const = 0;
    virtual int row() const = 0;
    ServiceModelItemBase * parent();
    virtual QList<ServiceModelItemBase*> getChildItems() const = 0;
    virtual bool hasChildren () const = 0;
    virtual QString getUrl() = 0;

    bool operator<( const ServiceModelItemBase& other ) const;
    

    /**
     * Used for forcing dynamic items to populate themselves. The default implementations does nothing
     */
    virtual int prePopulate() const { return 0; };
    virtual void populate() const {};


    KUrl::List getUrls();

    



protected:
     mutable QList<ServiceModelItemBase*> m_childItems;
     ServiceModelItemBase *m_parent;
};

#endif
