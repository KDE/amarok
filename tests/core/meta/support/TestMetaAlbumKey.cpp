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

#include "TestMetaAlbumKey.h"

#include "amarokconfig.h"
#include "config-amarok-test.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KLocalizedString>

QTEST_MAIN( TestMetaAlbumKey )

TestMetaAlbumKey::TestMetaAlbumKey()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

TestMetaAlbumKey::~TestMetaAlbumKey()
{
}

void
TestMetaAlbumKey::initTestCase()
{
    AmarokConfig::instance("amarokrc");

    // Artist Name - Amarok    Album Name - Amarok Test Album
    m_track1 = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(dataPath( "data/audio/album/Track01.ogg" )) );

    // Artist Name - Amarok    Album Name - Amarok Test Album 2
    m_track2 = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(dataPath( "data/audio/album2/Track01.ogg" )) );

    // Artist Name - Amarok 2   Album Name - Amarok Test Album 2
    m_track3 = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(dataPath( "data/audio/album2/Track02.ogg" )) );

    m_album1 = m_track1->album();
    m_album2 = m_track2->album();
    m_album3 = m_track3->album();
}

QString
TestMetaAlbumKey::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}

void
TestMetaAlbumKey::testAlbumKey()
{
    Meta::AlbumKey albumKey1( m_album1 );

    QCOMPARE( albumKey1.m_albumName, m_album1->name() );
    QCOMPARE( albumKey1.m_artistName, m_album1->albumArtist()->name() );
}

void
TestMetaAlbumKey::testOperatorAssignment()
{
    // For Constructor : AlbumKey( const AlbumPtr &album )
    Meta::AlbumKey albumKey1( m_album1 ), albumKey2( m_album2 ), tempAlbumKey;

    QVERIFY( !( albumKey1 == albumKey2 ) );

    tempAlbumKey = albumKey1;
    QCOMPARE( albumKey1, tempAlbumKey );

    // For Constructor : AlbumKey( const QString &name, const QString &artistName )
    Meta::AlbumKey albumKey3( "Artist 1", "Album 1" ), albumKey4( "Artist 2", "Album 2" );

    QVERIFY( !( albumKey1 == albumKey2 ) );

    tempAlbumKey = albumKey1;
    QCOMPARE( albumKey1, tempAlbumKey );
}

void
TestMetaAlbumKey::testOperatorLessThan()
{
    // For Constructor : AlbumKey( const AlbumPtr &album )
    Meta::AlbumKey albumKey1( m_album1 ), albumKey2( m_album2 ), albumKey3( m_album3 );

    // Same artist name, different album name
    QVERIFY( albumKey1 < albumKey2 );

    // Same artist name, same album name
    QVERIFY( !( albumKey1 < albumKey1 ) );

    // Different artist name, same album name
    QVERIFY( albumKey2 < albumKey3 );

    // Different artist name, different album name
    QVERIFY( albumKey1 < albumKey3 );

    // For Constructor : AlbumKey( const QString &name, const QString &artistName )
    Meta::AlbumKey albumKey4( QStringLiteral("Artist 1"), QStringLiteral("Album 1") ), albumKey5( QStringLiteral("Artist 1"), QStringLiteral("Album 2") ),
                   albumKey6( QStringLiteral("Artist 2"), QStringLiteral("Album 2") );

    // Same artist name, different album name
    QVERIFY( albumKey4 < albumKey5 );

    // Same artist name, same album name
    QVERIFY( !( albumKey4 < albumKey4 ) );

    // Different artist name, same album name
    QVERIFY( albumKey5 < albumKey6 );

    // Different artist name, different album name
    QVERIFY( albumKey4 < albumKey6 );
}
