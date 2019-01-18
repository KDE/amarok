/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2010-2013 Ralf Engels <ralf-engels@gmx.de>                             *
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

#ifndef TESTGENERICSCANMANAGER_H
#define TESTGENERICSCANMANAGER_H

#include "scanner/GenericScanManager.h"
#include "core/meta/support/MetaConstants.h"

#include <QSignalSpy>
#include <QTemporaryDir>

#include <QTest>

/** Test the GenericScanManager and the scanner job
 */
class TestGenericScanManager : public QObject
{
    Q_OBJECT

public:
    TestGenericScanManager();

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
     * Check that the scanner continues if crashed
     */
    void testRestartScanner();

    /**
     *  Test images
     */
    void testAlbumImage();


public Q_SLOTS:
    void slotStarted( GenericScanManager::ScanType type );
    void slotDirectoryCount( int count );
    void slotDirectoryScanned( QSharedPointer<CollectionScanner::Directory> dir );
    void slotSucceeded();
    void slotFailed( const QString& message );

private:
    void fullScanAndWait();
    void waitScannerFinished( QSignalSpy &spy );

    /**
       Creates a track in the m_tmpCollectionDir with the given values.
       Meta::valUrl gives the relative path to the target track.
    */
    void createTrack( const Meta::FieldHash &values );
    void createSingleTrack();
    void createAlbum();

    bool m_started;
    bool m_finished;

    int m_scannedDirsCount;
    int m_scannedTracksCount;
    int m_scannedCoversCount;

    QTemporaryDir *m_tmpCollectionDir;
    QString m_sourcePath; // the path to the template .mp3 file

    GenericScanManager *m_scanManager;
};

#endif // TESTGENERICSCANMANAGER_H
