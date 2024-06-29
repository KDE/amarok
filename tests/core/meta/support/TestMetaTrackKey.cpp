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

#include "TestMetaTrackKey.h"

#include "amarokconfig.h"
#include "config-amarok-test.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KLocalizedString>

QTEST_MAIN( TestMetaTrackKey )

TestMetaTrackKey::TestMetaTrackKey()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

QString
TestMetaTrackKey::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QLatin1Char('/') + relPath );
}

void
TestMetaTrackKey::testTrackKey()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));

    Meta::TrackPtr track;
    track = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(dataPath( QStringLiteral("data/audio/album/Track01.ogg") )) );

    Meta::TrackKey trackKey1( track );

    QCOMPARE( trackKey1.m_trackName, track->name() );
    QCOMPARE( trackKey1.m_discNumber, track->discNumber() );
    QCOMPARE( trackKey1.m_trackNumber, track->trackNumber() );
    QCOMPARE( trackKey1.m_artistName, track->artist()->name() );
    QCOMPARE( trackKey1.m_albumName, track->album()->name() );
}

void
TestMetaTrackKey::testOperatorAssignment()
{
    AmarokConfig::instance(QStringLiteral("amarokrc"));

    Meta::TrackPtr track;
    track = CollectionManager::instance()->trackForUrl( QUrl::fromLocalFile(dataPath( QStringLiteral("data/audio/album/Track01.ogg") )) );

    Meta::TrackKey trackKey1( track ), trackKey2;

    QVERIFY( !( trackKey1 == trackKey2 ) );

    trackKey2 = trackKey1;

    QCOMPARE( trackKey1, trackKey2 );
}
