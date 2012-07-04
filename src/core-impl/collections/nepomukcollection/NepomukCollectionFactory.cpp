/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#define DEBUG_PREFIX "NepomukCollection Factory"

#include <Nepomuk/ResourceManager>

#include "core/support/Debug.h"
#include "NepomukCollectionFactory.h"
#include "core/support/Debug.h"

using namespace Collections;

void
NepomukCollectionFactory::init()
{
    DEBUG_BLOCK
    // check if Nepomuk service is already initialized
    if( Nepomuk::ResourceManager::instance()->initialized() )
    {
        emit newCollection( new Collections::NepomukCollection() );
        m_initialized = true;
    }

    else
    {
        // Nepomuk not initialized, so initiate it
        if( Nepomuk::ResourceManager::instance()->init() )
        {
            emit newCollection( new Collections::NepomukCollection() );
            m_initialized = true;
        }

        else
        {
            warning() << "could not load nepomuk in collectionfactory";
            //TODO:
            // Generate appropriate warning since Nepomuk not found/enabled
        }
    }
}

NepomukCollectionFactory::NepomukCollectionFactory( QObject *parent,
        const QVariantList &args )
    : CollectionFactory( parent, args )
{
    DEBUG_BLOCK
    debug() << "in nepomukcollectionfactory";
    m_info = KPluginInfo( "amarok_collection-nepomukcollection.desktop", "services" );
}

NepomukCollectionFactory::~NepomukCollectionFactory()
{

}

AMAROK_EXPORT_COLLECTION( NepomukCollectionFactory, nepomukcollection )
