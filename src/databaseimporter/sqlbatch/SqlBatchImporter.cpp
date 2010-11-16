/****************************************************************************************
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "SqlBatchImporter.h"
#include "SqlBatchImporterConfig.h"

#include "core/support/Debug.h"
#include "core/capabilities/CollectionImportCapability.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KLocale>

SqlBatchImporter::SqlBatchImporter( QObject *parent )
    : DatabaseImporter( parent )
    , m_config( 0 )
{
}

SqlBatchImporter::~SqlBatchImporter()
{ }

DatabaseImporterConfig*
SqlBatchImporter::configWidget( QWidget *parent )
{
    if( !m_config )
        m_config = new SqlBatchImporterConfig( parent );
    return m_config;
}

void
SqlBatchImporter::import()
{
    DEBUG_BLOCK

    Q_ASSERT( m_config );
    if( !m_config )
    {
        error() << "No configuration exists, bailing out of import";
        return;
    }

    int numStarted = 0;
    // search for a collection with the CollectionImportCapability
    foreach( Collections::Collection *coll, CollectionManager::instance()->collections().keys() )
    {
        debug() << "Collection: "<<coll->prettyName() << "id:"<<coll->collectionId();
        QScopedPointer<Capabilities::CollectionImportCapability> cic( coll->create<Capabilities::CollectionImportCapability>());

        if( cic ) {

            QFile *file = new QFile( m_config->inputFilePath() );
            if( file->open( QIODevice::ReadOnly ) )
            {
                debug() << "importing db";
                cic->import( file, this );
                numStarted++;
            } else {
                debug() << "could not open";
                emit importError( i18n("Could not open file \"%1\".").arg( m_config->inputFilePath() ) );
                delete file;
            }
        }
    }

    if( !numStarted )
        emit importFailed();
}

