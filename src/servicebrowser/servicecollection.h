/* 
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef SERVICECOLLECTION_H
#define SERVICECOLLECTION_H

#include "collection.h"
#include "support/memorycollection.h"
#include "jamendo/jamendodatabasehandler.h"

#include <QtGlobal>

using namespace Meta;


class ServiceCollection : public Collection, public MemoryCollection
{
    Q_OBJECT
    public:
        ServiceCollection( );
        virtual ~ServiceCollection();

        virtual void startFullScan();
        virtual QueryMaker* queryBuilder();

        virtual QString collectionId() const;
        virtual QString prettyName() const;

};

#endif
