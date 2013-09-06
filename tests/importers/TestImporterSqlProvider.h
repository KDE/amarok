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

#ifndef TEST_IMPORTER_SQL_PROVIDER
#define TEST_IMPORTER_SQL_PROVIDER

#include <QObject>

class MockImporterSqlProvider;

class TestImporterSqlProvider : public QObject
{
    Q_OBJECT

private:
    void checkThread();

    MockImporterSqlProvider *m_mock;

private slots:
    void init();
    void cleanup();

    void constructorShouldCreateDatabaseConnection();
    void createdConnectionShouldHaveProperSettings();
    void createdSQLiteConnectionShouldHavePathSet();
    void createdNonSQLiteConnectionShouldHaveNameSet();
    void createdConnectionIsObjectUnique();

    void destructorShouldRemoveDatabaseConnection();

    void getArtistsShouldReceiveCreatedConnection();
    void getArtistTracksShouldReceiveCreatedConnection();

    void getArtistsShouldBeCalledInObjectsThread();
    void getArtistTracksShouldBeCalledInObjectsThread();

    void artistsShouldDelegateToGetArtists();
    void artistTracksShouldDelegateToGetArtistTracks();

    void artistsShouldCallPrepareConnectionBeforeGetArtists();
    void artistTracksShouldCallPrepareConnectionBeforeGetArtistTracks();
};

#endif // TEST_IMPORTER_SQL_PROVIDER
