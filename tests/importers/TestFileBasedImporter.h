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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef TEST_FILE_BASED_IMPORTER
#define TEST_FILE_BASED_IMPORTER

#include <QApplication>
#include <QExplicitlySharedDataPointer>
#include <QFileInfo>
#include <QObject>
#include <QVariantMap>

namespace StatSyncing
{
    class Provider;
    typedef QExplicitlySharedDataPointer<Provider> ProviderPtr;
}

class TestFileBasedImporterPrivate : public QObject
{
    Q_OBJECT

protected:
    virtual StatSyncing::ProviderPtr getProvider( const QString &db ) = 0;
    virtual StatSyncing::ProviderPtr getProvider() = 0;

    QVariantMap m_cfg;

private slots:
    void providerShouldHandleNonexistentDbFile();
    void providerShouldHandleInvalidDbFile();
    void providerShouldHandleErroneousConfigValues();
};

template<typename T>
class TestFileBasedImporter : public TestFileBasedImporterPrivate
{
protected:
    StatSyncing::ProviderPtr getProvider( const QString &db )
    {
        QVariantMap cfg( m_cfg );
        cfg.insert( "dbPath",
                    QFileInfo( db ).exists()
                    ? db
                    : QCoreApplication::applicationDirPath() + "/importers_files/" + db );

        return StatSyncing::ProviderPtr( new T( cfg, 0 ) );
    }

    StatSyncing::ProviderPtr getProvider()
    {
        return StatSyncing::ProviderPtr( new T( m_cfg, 0 ) );
    }
};

#endif // TEST_FILE_BASED_IMPORTER
