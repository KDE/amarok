/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#ifndef TESTSQLCOLLECTIONLOCATION_H
#define TESTSQLCOLLECTIONLOCATION_H

#include <QtTest>

#include <QTemporaryDir>

class MySqlEmbeddedStorage;
namespace Collections
{
    class SqlCollection;
}
class SqlRegistry;

class TestSqlCollectionLocation : public QObject
{
    Q_OBJECT
public:
    TestSqlCollectionLocation();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testOrganizingCopiesLabels();
    void testCopiesLabelFromExternalTracks();
    void testCopyTrackToDirectoryWithExistingTracks();

private:
    QString setupFileInTempDir( const QString &relativeName );

private:
    Collections::SqlCollection *m_collection;
    QSharedPointer<MySqlEmbeddedStorage> m_storage;
    static QTemporaryDir *s_tmpDir;
};

#endif // TESTSQLCOLLECTIONLOCATION_H
