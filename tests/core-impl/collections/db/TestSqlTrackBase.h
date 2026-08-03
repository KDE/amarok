/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef TESTSQLTRACKBASE_H
#define TESTSQLTRACKBASE_H

#include <QSharedPointer>
#include <QtTest>

#include <QTemporaryDir>

class SqlStorage;
class SqlRegistry;

namespace Collections {
    class SqlCollection;
}
namespace Meta {
    class SqlTrack;
}

class TestSqlTrackBase : public QObject
{
    Q_OBJECT

public:
    TestSqlTrackBase();

protected Q_SLOTS:
    virtual void initTestCase() = 0;
    virtual void cleanup() = 0;
private Q_SLOTS:
    void cleanupTestCase();

    void init();

    void testGetTrack();

    void testSetAllValuesSingleNotExisting();
    void testSetAllValuesSingleExisting();
    void testSetAllValuesBatch();

    void testUnsetValues();

    void testFinishedPlaying();

    void testAlbumRemainsCompilationAfterChangingAlbumName();
    void testAlbumRemaingsNonCompilationAfterChangingAlbumName();
    void testRemoveLabelFromTrack();
    void testRemoveLabelFromTrackWhenNotInCache();

    void testFullUtf8();

protected:
    void setAllValues( Meta::SqlTrack *track );
    void getAllValues( Meta::SqlTrack *track );

    Collections::SqlCollection *m_collection;
    QSharedPointer<SqlStorage> m_storage;
    static QTemporaryDir *s_tmpDir;
};

#endif // TESTSQLTRACKBASE_H
