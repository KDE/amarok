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

#include "NepomukCollection.h"
#include "NepomukCollectionFactory.h"

#include "core/logger/Logger.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"

#include <Nepomuk2/ResourceManager>

void
NepomukCollectionFactory::init()
{
    DEBUG_BLOCK
    m_initialized = true;

    // check if Nepomuk service is running
    if( Nepomuk2::ResourceManager::instance()->initialized() )
    {
        // if it is, create a new NepomukCollection
        emit newCollection( new Collections::NepomukCollection() );
        return;
    }
    else
    {
        warning() << "Couldn't initialize Nepomuk Collection. "
                  "Check if 'Nepomuk Semantic Desktop' is enabled in System Settings -> Desktop Search. "
                  "Nepomuk Plugin won't be loaded unless Nepomuk is enabled.";

        Amarok::Logger::longMessage(
            i18n( "Couldn't initialize Nepomuk Collection. "
                  "Check if 'Nepomuk Semantic Desktop' is enabled in System Settings -> Desktop Search. "
                  "Nepomuk Plugin won't be loaded unless Nepomuk is enabled." ),
            Amarok::Logger::Warning );
    }
}

NepomukCollectionFactory::NepomukCollectionFactory( QObject *parent,
        const QVariantList &args )
    : CollectionFactory( parent, args )
{
    m_info = KPluginInfo( "amarok_collection-nepomukcollection.desktop" );
}

NepomukCollectionFactory::~NepomukCollectionFactory()
{
}

AMAROK_EXPORT_COLLECTION( NepomukCollectionFactory, nepomukcollection )
