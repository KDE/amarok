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

#ifndef TESTSQLQUERYMAKER_H
#define TESTSQLQUERYMAKER_H

#include <QtTest/QTest>

#include <KTempDir>

class SqlStorage;
class SqlMountPointManagerMock;

namespace Collections {
    class SqlCollection;
}

class TestSqlQueryMaker : public QObject
{
    Q_OBJECT
public:
    TestSqlQueryMaker();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void cleanup();

    void testQueryTracks();
    void testQueryAlbums();
    void testQueryGenres();
    void testQueryYears();
    void testQueryComposers();
    void testQueryArtists();
    void testAlbumQueryMode();

    void testDeleteQueryMakerWithRunningQuery();

    void testAsyncTrackQuery();
    void testAsyncArtistQuery();
    void testAsyncGenreQuery();
    void testAsyncComposerQuery();
    void testAsyncAlbumQuery();
    void testAsyncYearQuery();
    void testAsyncTrackDataQuery();
    void testAsyncCustomQuery();

    void testFilter();
    void testFilter_data();

    void testMatch();
    void testMatch_data();

    void testDynamicCollection();

    void testSpecialCharacters_data();
    void testSpecialCharacters();

    void testNumberFilter();
    void testNumberFilter_data();

    void testReturnFunctions_data();
    void testReturnFunctions();

    void testLabelMatch();
    void testMultipleLabelMatches();

    void testQueryTypesWithLabelMatching_data();
    void testQueryTypesWithLabelMatching();

    void testFilterOnLabelsAndCombination();
    void testFilterOnLabelsOrCombination();
    void testFilterOnLabelsNegationAndCombination();
    void testFilterOnLabelsNegationOrCombination();
    void testComplexLabelsFilter();

    void testLabelQueryMode_data();
    void testLabelQueryMode();

private:
    Collections::SqlCollection *m_collection;
    SqlMountPointManagerMock *m_mpm;
    SqlStorage *m_storage;
    KTempDir *m_tmpDir;
};

#endif // TESTSQLQUERYMAKER_H
