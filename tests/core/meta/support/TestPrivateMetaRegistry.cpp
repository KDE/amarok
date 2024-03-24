/****************************************************************************************
 * Copyright (c) 2012 Jasneet Singh Bhatti <jazneetbhatti@gmail.com>                    *
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

#include "TestPrivateMetaRegistry.h"

#include "amarokconfig.h"
#include "config-amarok-test.h"
#include "core/meta/Meta.h"
#include "core/meta/support/PrivateMetaRegistry.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KLocalizedString>

using namespace Meta;

QTEST_MAIN( TestPrivateMetaRegistry )


TestPrivateMetaRegistry::TestPrivateMetaRegistry()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

TestPrivateMetaRegistry::~TestPrivateMetaRegistry()
{
}

void
TestPrivateMetaRegistry::initTestCase()
{
    AmarokConfig::instance("amarokrc");

    m_track1 = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(datapath( "data/audio/album/Track01.ogg" )) );
    m_track2 = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(datapath( "data/audio/album/Track02.ogg" )) );
    m_track3 = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(datapath( "data/audio/Platz 01.mp3" )) );
    m_track4 = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(datapath( "data/audio/Platz 02.mp3" )) );
    m_track5 = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(datapath( "data/audio/Platz 03.mp3" )) );
}

QString
TestPrivateMetaRegistry::datapath( const QString& relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void
TestPrivateMetaRegistry::createTestData()
{
    QTest::addColumn<QString>( "owner" );
    QTest::addColumn<QString>( "key" );
    QTest::addColumn<TrackPtr>( "track" );
    QTest::addColumn<AlbumPtr>( "album" );
    QTest::addColumn<ArtistPtr>( "artist" );
    QTest::addColumn<ComposerPtr>( "composer" );
    QTest::addColumn<GenrePtr>( "genre" );
    QTest::addColumn<YearPtr>( "year" );

    QTest::newRow( "Track01.ogg" ) << "Amarok" << "Amarok Test Album" << m_track1 << m_track1->album()
                                   << m_track1->artist() << m_track1->composer()
                                   << m_track1->genre() << m_track1->year();

    QTest::newRow( "Track02.ogg" ) << "Amarok" << "Amarok Test Album" << m_track2 << m_track2->album()
                                   << m_track2->artist() << m_track2->composer()
                                   << m_track2->genre() << m_track2->year();

    QTest::newRow( "Platz 01.mp3" ) << "Amarok" << "No album" << m_track3 << m_track3->album()
                                   << m_track3->artist() << m_track3->composer()
                                   << m_track3->genre() << m_track3->year();

    QTest::newRow( "Platz 02.mp3" ) << "Amarok" << "No album" << m_track4 << m_track4->album()
                                   << m_track4->artist() << m_track4->composer()
                                   << m_track4->genre() << m_track4->year();

    QTest::newRow( "Platz 03.mp3" ) << "owner" << "No album" << m_track5 << m_track5->album()
                                   << m_track5->artist() << m_track5->composer()
                                   << m_track5->genre() << m_track5->year();
}

void
TestPrivateMetaRegistry::testInsertAlbum_data()
{
    createTestData();
}

void
TestPrivateMetaRegistry::testInsertAlbum()
{
    QFETCH( QString, owner );
    QFETCH( QString, key );
    QFETCH( AlbumPtr, album );
    PrivateMetaRegistry::instance()->insertAlbum( owner, key, album );

    QCOMPARE( PrivateMetaRegistry::instance()->album( owner, key ), album );
}

void
TestPrivateMetaRegistry::testInsertArtist_data()
{
    createTestData();
}

void
TestPrivateMetaRegistry::testInsertArtist()
{
    QFETCH( QString, owner );
    QFETCH( QString, key );
    QFETCH( ArtistPtr, artist );
    PrivateMetaRegistry::instance()->insertArtist( owner, key, artist );

    QCOMPARE( PrivateMetaRegistry::instance()->artist( owner, key ), artist );
}

void
TestPrivateMetaRegistry::testInsertComposer_data()
{
    createTestData();
}

void
TestPrivateMetaRegistry::testInsertComposer()
{
    QFETCH( QString, owner );
    QFETCH( QString, key );
    QFETCH( ComposerPtr, composer );
    PrivateMetaRegistry::instance()->insertComposer( owner, key, composer );

    QCOMPARE( PrivateMetaRegistry::instance()->composer( owner, key ), composer );
}

void
TestPrivateMetaRegistry::testInsertGenre_data()
{
    createTestData();
}

void
TestPrivateMetaRegistry::testInsertGenre()
{
    QFETCH( QString, owner );
    QFETCH( QString, key );
    QFETCH( GenrePtr, genre );
    PrivateMetaRegistry::instance()->insertGenre( owner, key, genre );

    QCOMPARE( PrivateMetaRegistry::instance()->genre( owner, key ), genre );
}

void
TestPrivateMetaRegistry::testInsertYear_data()
{
    createTestData();
}

void
TestPrivateMetaRegistry::testInsertYear()
{
    QFETCH( QString, owner );
    QFETCH( QString, key );
    QFETCH( YearPtr, year );
    PrivateMetaRegistry::instance()->insertYear( owner, key, year );

    QCOMPARE( PrivateMetaRegistry::instance()->year( owner, key ), year );
}

void
TestPrivateMetaRegistry::testNull()
{
    QVERIFY( !PrivateMetaRegistry::instance()->album( "invalid", "invalid" ) );
    QVERIFY( !PrivateMetaRegistry::instance()->artist( "invalid", "invalid" ) );
    QVERIFY( !PrivateMetaRegistry::instance()->composer( "invalid", "invalid" ) );
    QVERIFY( !PrivateMetaRegistry::instance()->genre( "invalid", "invalid" ) );
    QVERIFY( !PrivateMetaRegistry::instance()->year( "invalid", "invalid" ) );
}
