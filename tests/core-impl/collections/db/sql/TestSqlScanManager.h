/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010, 2013 Ralf Engels <ralf-engels@gmx.de>                            *
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

#include <QSharedPointer>
#include <QTest>

#include "core/meta/support/MetaConstants.h"

#include <QTemporaryDir>

class MySqlEmbeddedStorage;
class GenericScanManager;

class QIODevice;

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

Q_SIGNALS:
    void scanManagerResult();

private Q_SLOTS:
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
     * Check that duplicate uids are handled correctly
     */
    void testDuplicateUid();

    /**
     * Check that very long uids don't produce wrong sql queries
     */
    void testLongUid();


    /**
     * Check that detecting compilations works
     * Test also compilation/no compilation tags
     */
    void testCompilation();

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
     *  Will also check that the statistics are not deleted.
     */
    void testRemoveTrack();

    /**
     *  Test rescanning and detecting a moved track or directory
     */
    void testMove();

    /**
     *  Test "feat" artist and albums
     */
    void testFeat();

    /**
     *  Test images
     */
    void testAlbumImage();

    /**
     * Test merging of the result with an incremental scan.
     * New files should be added to existing albums.
     * Existing files should be merged.
     */
    void testMerges();

    void testLargeInsert();

    void testIdentifyCompilationInMultipleDirectories();

    void testUidChangeMoveDirectoryIncrementalScan();

    /** Test that Amarok respects the album artist from the tags.
     *  (BR: 216759)
     */
    void testAlbumArtistMerges();

    /** Test that two tracks getting their filename exchanged are still
     *  handled correctly, ratings and all (BR: 272802)
     */
    void testCrossRenaming();

    /** Test the partial update scan functionality to ensure e.g. correct handling
     *  of paths (BR: 475528)
     */
    void testPartialUpdate();

    void slotCollectionUpdated();

private:
    void fullScanAndWait();
    void incrementalScanAndWait( const QList<QUrl> &paths = QList<QUrl>() );
    void importAndWait( QIODevice* import );

    void waitScannerFinished();

    /**
       Creates a track in the m_tmpCollectionDir with the given values.
       Meta::valUrl gives the relative path to the target track.
    */
    void createTrack( const Meta::FieldHash &values );
    void createSingleTrack();

    /** Creates a default album.
      Meaning that album artist tag is not set, all songs are in one directory and the artist is the same for all the tracks.
    */
    void createAlbum();
    void createCompilation();
    void createCompilationTrack();

    /**
     * Album that looks alike compilation: various track's artists
     * but single album artist and no compilation flag.
     */
    void createCompilationLookAlikeAlbum();

    int m_collectionUpdatedCount;

    QSharedPointer<MySqlEmbeddedStorage> m_storage;
    static QTemporaryDir *s_tmpDatabaseDir;
    QTemporaryDir *m_tmpCollectionDir;
    QString m_sourcePath; // the path to the template .mp3 file
    bool m_autoGetCoverArt;

    Collections::SqlCollection *m_collection;
    GenericScanManager *m_scanManager;
};

#endif // TESTSQLSCANMANAGER_H
