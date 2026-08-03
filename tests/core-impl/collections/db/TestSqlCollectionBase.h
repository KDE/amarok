/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2026 Tuomas Nurmi <tuomas@norsumanageri.org>                           *
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

#ifndef TESTSQLCOLLECTIONBASE_H
#define TESTSQLCOLLECTIONBASE_H

#include <QSharedPointer>
#include <QtTest>

#include <QTemporaryDir>

class SqlMountPointManagerMock;
class SqlStorage;

namespace Collections {
    class SqlCollection;
}

class TestSqlCollectionBase : public QObject
{
    Q_OBJECT

public:
    TestSqlCollectionBase();

protected Q_SLOTS:
    virtual void initTestCase() = 0;
private Q_SLOTS:
    void cleanupTestCase();

    void testDeviceAddedWithTracks();
    void testDeviceAddedWithoutTracks();
    void testDeviceRemovedWithTracks();
    void testDeviceRemovedWithoutTracks();

protected:
    Collections::SqlCollection *m_collection;
    SqlMountPointManagerMock *m_mpmMock;
    QSharedPointer<SqlStorage> m_storage;
    static QTemporaryDir *s_tmpDir;
};

#endif // TESTSQLCOLLECTIONBASE_H
