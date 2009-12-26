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

#ifndef TESTSQLTRACKEDITING_H
#define TESTSQLTRACKEDITING_H

#include <QtTest/QtTest>

#include <KTempDir>

class SqlStorage;
class SqlCollection;
class SqlRegistry;

class TestSqlTrackEditing : public QObject
{
    Q_OBJECT
public:
    TestSqlTrackEditing();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testChangeGenreToExisting();
    void testChangeGenreToNew();
    void testChangeComposerToExisting();
    void testChangeComposerToNew();
    void testChangeYearToExisting();
    void testChangeYearToNew();

private:
    SqlCollection *m_collection;
    SqlStorage *m_storage;
    KTempDir *m_tmpDir;
    SqlRegistry *m_registry;
};

QTEST_MAIN( TestSqlTrackEditing )

#endif // TESTSQLTRACKEDITING_H
