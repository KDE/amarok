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



#define DEBUG_PREFIX "ServiceCollection"

#include "servicecollection.h"

#include "amarokconfig.h"
#include "servicemetabase.h"
#include "debug.h"
#include "support/memoryquerymaker.h"
//#include "reader.h"

#include <QStringList>
#include <QTimer>

using namespace Meta;


//ServiceCollection

ServiceCollection::ServiceCollection( )
    : Collection()
    , MemoryCollection()
{



}

ServiceCollection::~ServiceCollection()
{

}

void
ServiceCollection::startFullScan()
{
    //ignore
}

QueryMaker*
ServiceCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString
ServiceCollection::collectionId() const
{
    return "service collection";
}

QString
ServiceCollection::prettyName() const
{
    return "service collection";
}


#include "servicecollection.moc"

