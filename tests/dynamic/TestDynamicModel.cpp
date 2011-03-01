/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "TestDynamicModel.h"

#include "dynamic/Bias.h"
#include "dynamic/BiasedPlaylist.h"
#include "dynamic/DynamicModel.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <KTempDir>

#include <qtest_kde.h>

KTempDir *s_tmpDir = 0;

// We return a special saveLocation.
QString Amarok::saveLocation( const QString &directory )
{
    return s_tmpDir->name() + directory;
}

QTEST_KDEMAIN_CORE( TestDynamicModel )

TestDynamicModel::TestDynamicModel()
{
}

void
TestDynamicModel::init()
{
    s_tmpDir = new KTempDir();
}

void
TestDynamicModel::cleanup()
{
    delete s_tmpDir;
}

void
TestDynamicModel::testData()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadCurrentPlaylists();

    // now we should have the default playlist
    QCOMPARE( model->columnCount(), 1 );
    QCOMPARE( model->rowCount(), 1 );
    QCOMPARE( model->data( model->index(0, 0) ).toString(), QString("Random") );
}

void
TestDynamicModel::testPlaylistIndex()
{
}

void
TestDynamicModel::testSaveActive()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadCurrentPlaylists();

    // now we should have the default playlist
    QCOMPARE( model->isActiveUnsaved(), false );
    QCOMPARE( model->isActiveDefault(), true );
    QCOMPARE( model->activePlaylistIndex(), 0 );
    QCOMPARE( model->defaultPlaylistIndex(), model->activePlaylistIndex() );
    QCOMPARE( model->activePlaylist()->title(), QString("Random") );

    // change the default playlist
    Dynamic::BiasedPlaylist* playlist = qobject_cast<Dynamic::BiasedPlaylist*>(model->activePlaylist());
    playlist->bias()->replace( Dynamic::BiasPtr( new Dynamic::UniqueBias() ) );

    QCOMPARE( model->isActiveUnsaved(), true );
    QVERIFY( model->data( model->index(0, 0) ).toString() != QString("Random") );

    // saving and loading the playlist does not change the state
    model->saveCurrentPlaylists();
    QCOMPARE( model->isActiveUnsaved(), true );
    QCOMPARE( model->activePlaylistIndex(), 0 );

    model->loadCurrentPlaylists();
    QCOMPARE( model->isActiveUnsaved(), true );
    QCOMPARE( model->activePlaylistIndex(), 0 );

    model->saveActive( "New Playlist" );

    // Saved the playlist under a new name (at the end)
    QCOMPARE( model->columnCount(), 1 );
    QCOMPARE( model->rowCount(), 2 );
    QCOMPARE( model->data( model->index(model->activePlaylistIndex(), 0) ).toString(),
              QString("New Playlist") );
    QCOMPARE( model->isActiveUnsaved(), false );
    QCOMPARE( model->isActiveDefault(), false );
    QCOMPARE( model->activePlaylistIndex(), 1 );
    QVERIFY( model->defaultPlaylistIndex() != model->activePlaylistIndex() );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist") );

    // Save the unmodified playlist under a new name
    model->saveActive( "New Playlist 2" );
    QCOMPARE( model->rowCount(), 3 );
    QCOMPARE( model->isActiveUnsaved(), false );
    QCOMPARE( model->isActiveDefault(), false );
    QCOMPARE( model->activePlaylistIndex(), 2 );

    // change the default playlist again
    model->setActivePlaylist( model->defaultPlaylistIndex() );
    playlist = qobject_cast<Dynamic::BiasedPlaylist*>(model->activePlaylist());
    playlist->bias()->replace( Dynamic::BiasPtr( new Dynamic::UniqueBias() ) );

    QCOMPARE( model->isActiveUnsaved(), true );

    // Saved the playlist under an already existing
    model->saveActive( "New Playlist" );
    QCOMPARE( model->rowCount(), 3 );
    QCOMPARE( model->data( model->index(model->activePlaylistIndex(), 0) ).toString(),
              QString("New Playlist") );
    QCOMPARE( model->isActiveUnsaved(), false );
    QCOMPARE( model->isActiveDefault(), false );
    QCOMPARE( model->activePlaylistIndex(), 2 );
    QVERIFY( model->defaultPlaylistIndex() != model->activePlaylistIndex() );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist") );
}

void
TestDynamicModel::testRemoveActive()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadCurrentPlaylists();

    // create three new playlists
    model->saveActive( "New Playlist" );
    model->saveActive( "New Playlist 2" );
    model->saveActive( "New Playlist 3" );

    QCOMPARE( model->rowCount(), 4 );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist 3") );

    // try to remove the default playlist
    model->setActivePlaylist( model->defaultPlaylistIndex() );
    model->removeActive();
    QCOMPARE( model->rowCount(), 4 );

    // try to remove the the second
    model->setActivePlaylist( 2 );
    model->removeActive();
    QCOMPARE( model->rowCount(), 3 );

    model->setActivePlaylist( 1 );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist") );

    model->setActivePlaylist( 2 );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist 3") );
}

#include "TestDynamicModel.moc"
