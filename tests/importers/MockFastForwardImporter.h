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

#ifndef MOCK_FAST_FORWARD_IMPORTER_H
#define MOCK_FAST_FORWARD_IMPORTER_H

#include "databaseimporter/amarok14/FastForwardImporter.h"

#include "core/collections/Collection.h"
#include "core/collections/CollectionLocation.h"
#include "databaseimporter/amarok14/FastForwardImporterConfig.h"

#include <QCoreApplication>

class MockFastForwardImporter : public FastForwardImporter
{
public:
    MockFastForwardImporter( QObject *parent = 0 )
        : FastForwardImporter( parent )\
    {
        m_config = new MockFastForwardImporterConfig;
    }

    ~MockFastForwardImporter()
    {
        delete m_config;
        m_config = 0;
    }

private:
    class MockFastForwardImporterConfig : public FastForwardImporterConfig
    {
    public:
        FastForwardImporter::ConnectionType connectionType() const
        {
            return FastForwardImporter::SQLite;
        }

        QString databaseLocation() const
        {
            return QCoreApplication::applicationDirPath()
                    + "/importers_files/collection.db";
        }

        QString databaseName() const
        {
            return databaseLocation();
        }

        QString databaseHost() const
        {
            return QString( "localhost" );
        }

        QString databaseUser() const
        {
            return QString();
        }

        QString databasePass() const
        {
            return QString();
        }

        bool smartMatch() const
        {
            return true;
        }

        bool importArtwork() const
        {
            return false;
        }

        QString importArtworkDir() const
        {
            return QString();
        }
    };
};

#endif // MOCK_FAST_FORWARD_IMPORTER_H
