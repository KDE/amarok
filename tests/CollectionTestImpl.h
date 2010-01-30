/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef COLLECTIONTESTIMPL_H
#define COLLECTIONTESTIMPL_H

#include "collection/Collection.h"
#include "collection/support/MemoryCollection.h"
#include "collection/support/MemoryQueryMaker.h"

//simple Amarok::Collection implementation based on MemoryCollection

class CollectionLocationTestImpl;

class CollectionTestImpl : public Amarok::Collection, public MemoryCollection
{
public:
    CollectionTestImpl( const QString &collectionId )
        : Amarok::Collection(), MemoryCollection()
    { this->id = collectionId; }

    QueryMaker* queryMaker() { return new MemoryQueryMaker( this, id ); }

    KIcon icon() const { return KIcon(); }

    QString collectionId() const { return id; }
    QString prettyName() const { return id; }

    QString id;

};

#endif
