/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "TestArtistHelper.h"

#include "ArtistHelper.h"

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestArtistHelper )

TestArtistHelper::TestArtistHelper() : QObject()
{

}

void
TestArtistHelper::testRealTrackArtist_data()
{
    QTest::addColumn<QString>( "artistTag" );
    QTest::addColumn<QString>( "realArtist" );

    QTest::newRow( "no ft." ) << "Artist A" << "Artist A";
    QTest::newRow( "A ft. B") << "A ft. B" << "A";
    QTest::newRow( "A feat. B" ) << "A feat. B" << "A";
    QTest::newRow( "A featuring B" ) << "A featuring B" << "A";
    QTest::newRow( "A f. B" ) << "A f. B" << "A";
    QTest::newRow( "artist including ft. string" ) << "Aft.B" << "Aft.B";
}

void
TestArtistHelper::testRealTrackArtist()
{
    QFETCH( QString, artistTag );
    QFETCH( QString, realArtist );

    QCOMPARE( ArtistHelper::realTrackArtist( artistTag ), realArtist );
}

#include "TestArtistHelper.moc"
