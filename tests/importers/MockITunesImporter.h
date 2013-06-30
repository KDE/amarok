/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MOCK_ITUNES_IMPORTER_H
#define MOCK_ITUNES_IMPORTER_H

#include "databaseimporter/itunes/ITunesImporter.h"

#include "core/collections/Collection.h"
#include "core/collections/CollectionLocation.h"
#include "databaseimporter/itunes/ITunesImporterConfig.h"

#include <QCoreApplication>

class MockITunesImporter : public ITunesImporter
{
public:
    MockITunesImporter( Collections::Collection *collection, QObject *parent = 0 )
        : ITunesImporter( parent )
    {
        m_config = new MockITunesImporterConfig( collection );
    }

    ~MockITunesImporter()
    {
        delete m_config;
        m_config = 0;
    }

private:
    class MockITunesImporterConfig : public ITunesImporterConfig
    {
    public:
        MockITunesImporterConfig( Collections::Collection *collection )
            : ITunesImporterConfig()
            , m_collectionLocation( collection->location() )
        {
        }

        QString databaseLocation() const
        {
            return QCoreApplication::applicationDirPath()
                    + "/importers_files/iTunes Music Library.xml";
        }

        Collections::CollectionLocation *collectionLocation() const
        {
            return m_collectionLocation;
        }

    private:
        Collections::CollectionLocation * const m_collectionLocation;
    };
};

#endif // MOCK_ITUNES_IMPORTER_H
