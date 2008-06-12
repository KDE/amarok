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

#define DEBUG_PREFIX "NepomukCollection"

#include "NepomukCollection.h"
#include "NepomukQueryMaker.h"

#include "Debug.h"

#include <klocale.h>
#include <Nepomuk/ResourceManager>


AMAROK_EXPORT_PLUGIN( NepomukCollectionFactory )

// CollectionFactory

void
NepomukCollectionFactory::init()
{
    Soprano::Client::DBusClient* client = new Soprano::Client::DBusClient( "org.kde.NepomukStorage" );

    // TODO: use QLocalSocket 
    //if ( Nepomuk::ResourceManager::instance()->init() == 0 )
    if (client->isValid())
    {
        Collection* collection = new NepomukCollection(client);
        emit newCollection( collection );
    }
    else
    {
        warning() << "Nepomuk is not running, can not init Nepomuk Collection" << endl;
        delete client;
    }
}

// NepomukCollection

NepomukCollection::NepomukCollection(Soprano::Client::DBusClient *client)
    :   Collection() 
    ,   m_client( client )
{
    
}

NepomukCollection::~NepomukCollection()
{
    delete m_client;
}

QueryMaker*
NepomukCollection::queryMaker()
{
	return new NepomukQueryMaker(this, m_client);
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

void
NepomukCollection::lostDBusConnection()
{
    debug() << "removing NepomukCollection, lost dbus connection" << endl;
    emit remove();
}


#include "NepomukCollection.moc"
