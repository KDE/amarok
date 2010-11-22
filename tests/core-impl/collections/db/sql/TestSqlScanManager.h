/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef TESTSQLSCANMANAGER_H
#define TESTSQLSCANMANAGER_H

#include <QtTest/QTest>

#include "core/meta/support/MetaConstants.h"

#include <KTempDir>

class SqlStorage;
class ScanManager;

namespace Collections {
    class SqlCollection;
}

/** Test the ScanManager, the SqlScanResultProcessor and the amarokcollectionscanner itself.
    Note: currently this test is using the installed amarokcollectionscanner and not
     the one from util/collectionscanner.
 */
class TestSqlScanManager : public QObject
{
    Q_OBJECT
public:
    TestSqlScanManager();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    /**
     * Check that a single insert really inserts all the information
     */
    void testScanSingle();

    /**
     * Check that fully scanning a directory works
     */
    void testScanDirectory();

    /**
     * Check that detecting compilations works
     */
    void testCompilation();

    /**
     * Check that the scanner continues if crashed
     */
    void testRestartScanner();

    /**
     * Check that a blocked scan really does nothing.
     */
    void testBlock();

    /**
     *  Test adding a whole directory (incremental scan)
     */
    void testAddDirectory();

    /**
     *  Test rescanning and detecting a removed directory (incremental)
     */
    void testRemoveDir();

    /**
     *  Test rescanning and detecting a removed track (incremental)
     */
    void testRemoveTrack();

    /**
     * Test merging of the result with an incremental scan.
     * New files should be added to existing albums.
     * Existing files should be merged.
     */
    void testMerges();

    void testLargeInsert();
    void testIdentifyCompilationInMultipleDirectories();

private:
    void waitScannerFinished();
    void createTrack( const Meta::FieldHash &values );
    void createSingleTrack();
    void createAlbum();
    void createCompilation();
    void createCompilationTrack();

    SqlStorage *m_storage;
    KTempDir *m_tmpDatabaseDir;
    KTempDir *m_tmpCollectionDir;
    QString m_sourcePath; // the path to the template .mp3 file

    Collections::SqlCollection *m_collection;
    ScanManager *m_scanManager;
};

#endif // TESTSQLSCANMANAGER_H
