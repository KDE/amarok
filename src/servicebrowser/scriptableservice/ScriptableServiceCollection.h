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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/ 
 
#ifndef SCRIPTABLESERVICECOLLECTION_H
#define SCRIPTABLESERVICECOLLECTION_H

#include "collection.h"
#include "support/memorycollection.h"

/**
A collection for use by the scriptable service. Stores everything in memory

	@author 
*/
class ScriptableServiceCollection : public Collection, public MemoryCollection
{
public:
    ScriptableServiceCollection( const QString &name );

    virtual ~ScriptableServiceCollection();

    virtual QueryMaker * queryMaker();
    virtual void startFullScan() { }

    virtual QString collectionId() const;
    virtual QString prettyName() const;
  
    virtual bool possiblyContainsTrack( const KUrl &url ) const;
    virtual Meta::TrackPtr trackForUrl( const KUrl &url );

    virtual CollectionLocation* location() const;

    void emitUpdated(); 

    

private:

    QString m_name;

};

#endif
