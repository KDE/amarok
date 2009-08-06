/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "DirectoryLoader.h"
#include "TestDirectoryLoader.h"

#include "playlist/PlaylistController.h"
#include "playlist/PlaylistModel.h"

#include <KStandardDirs>

/* This one is a bit ugly, as the results of the methods in DirectoryLoader can't *
 * be checked directly there but only in the playlist.                            */

TestDirectoryLoader::TestDirectoryLoader( QStringList testArgumentList )
{
    testArgumentList.replace( 2, testArgumentList.at( 2 ) + "DirectoryLoader.log" );
    QTest::qExec( this, testArgumentList );
}

void TestDirectoryLoader::initTestCase()
{
    The::playlistController()->clear(); // we need a clear playlist for those tests

    DirectoryLoader *loader1 = new DirectoryLoader;
    DirectoryLoader *loader2 = new DirectoryLoader;
    QList<QUrl> testList;
    QUrl testUrl;

    testUrl = QUrl::fromLocalFile( KStandardDirs::installPath( "data" ) + "amarok/testdata/audio/" );
    testList.append( testUrl );

    loader1->insertAtRow( 1 ); // TODO: negative values always seem to append at the beginning. is that correct?
    loader1->init( testList );
    // wait until finished... HOW? might only work with --nofork?
    loader2->insertAtRow( 4 );
    loader2->init( testList );
    // here we should wait again
}

void TestDirectoryLoader::testInitAndInsertAtRow()
{
    /* more uglyness: we test both methods at once... I don't see another way */
    QCOMPARE( The::playlistModel()->rowCount(), 20 );

    QCOMPARE( The::playlistModel()->trackAt( 1 )->prettyName(), QString( "" ) ); // TODO
    QCOMPARE( The::playlistModel()->trackAt( 4 )->prettyName(), QString( "" ) );
    QCOMPARE( The::playlistModel()->trackAt( 5 )->prettyName(), QString( "" ) );
    QCOMPARE( The::playlistModel()->trackAt( 14 )->prettyName(), QString( "" ) );
    QCOMPARE( The::playlistModel()->trackAt( 20 )->prettyName(), QString( "" ) );
}
