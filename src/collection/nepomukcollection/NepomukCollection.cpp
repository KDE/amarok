/* 
   Copyright (C) 2008 Daniel Winter <dw@danielwinter.de>

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

#include "NepomukCollection.h"
#include "NepomukQueryMaker.h"

#include <klocale.h>


AMAROK_EXPORT_PLUGIN( NepomukCollectionFactory )

// CollectionFactory

void
NepomukCollectionFactory::init()
{
	// TODO: Check if Nepomuk is running
	Collection* collection = new NepomukCollection();
	emit newCollection( collection );
}

// NepomukCollection

NepomukCollection::NepomukCollection()
{

}

NepomukCollection::~NepomukCollection()
{
}

QueryMaker* 
NepomukCollection::queryMaker()
{
	return new NepomukQueryMaker();
}

QString
NepomukCollection::collectionId() const
{
	return "nepomukCollection";
}

QString
NepomukCollection::prettyName() const
{
	return i18n("Nepomuk Collection");
}

#include "NepomukCollection.moc"