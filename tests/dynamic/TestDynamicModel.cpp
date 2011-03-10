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

#include <QDebug>

#include <QSignalSpy>
#include <KTempDir>

#include <qtest_kde.h>

Q_DECLARE_METATYPE(QModelIndex);

KTempDir *s_tmpDir = 0;

// We return a special saveLocation.
QString Amarok::saveLocation( const QString &directory )
{
    return s_tmpDir->name() + directory;
}

QTEST_KDEMAIN_CORE( TestDynamicModel )

TestDynamicModel::TestDynamicModel()
{
    qRegisterMetaType<QModelIndex>();
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

    // now we should have the four default playlists
    QModelIndex playlistIndex = model->index( 0, 0 );
    QCOMPARE( model->data( playlistIndex ).toString(), QString("Random") );
    QCOMPARE( model->data( playlistIndex, Qt::EditRole ).toString(), QString("Random") );
    QVERIFY(  model->data( playlistIndex, Dynamic::DynamicModel::PlaylistRole ).isValid() );
    QVERIFY( !model->data( playlistIndex, Dynamic::DynamicModel::BiasRole ).isValid() );

    QModelIndex biasIndex = model->index( 0, 0, playlistIndex );
    QVERIFY( !model->data( biasIndex ).toString().isEmpty() );
    QVERIFY( !model->data( biasIndex, Dynamic::DynamicModel::PlaylistRole ).isValid() );
    QVERIFY(  model->data( biasIndex, Dynamic::DynamicModel::BiasRole ).isValid() );
}

void
TestDynamicModel::testPlaylistIndex()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadCurrentPlaylists();

    // now we should have the four default playlists
    QCOMPARE( model->rowCount(), 4 );
    QCOMPARE( model->columnCount(), 1 );

    // -- random playlist with one bias
    QModelIndex playlistIndex = model->index( 0, 0 );
    QModelIndex biasIndex = model->index( 0, 0, playlistIndex );

    QCOMPARE( model->rowCount( playlistIndex ), 1 );
    QCOMPARE( model->rowCount( biasIndex ), 0 );
    QCOMPARE( playlistIndex.parent(), QModelIndex() );
    QCOMPARE( biasIndex.parent(), playlistIndex );

    // -- albumplay playlist with bias structure
    playlistIndex = model->index( 2, 0 );
    biasIndex = model->index( 0, 0, playlistIndex );
    QModelIndex subBiasIndex = model->index( 0, 0, biasIndex );

    QCOMPARE( model->rowCount( playlistIndex ), 1 );
    QCOMPARE( model->rowCount( biasIndex ), 2 );
    QCOMPARE( model->rowCount( subBiasIndex ), 0 );
    QCOMPARE( playlistIndex.parent(), QModelIndex() );
    QCOMPARE( biasIndex.parent(), playlistIndex );
    QCOMPARE( subBiasIndex.parent(), biasIndex );


    // and now the non-model index functions:
    model->setActivePlaylist( 2 );
    Dynamic::DynamicPlaylist* playlist = model->activePlaylist();
    playlistIndex = model->index( model->activePlaylistIndex(), 0 );
    QCOMPARE( model->index( playlist ), playlistIndex );

    Dynamic::BiasPtr bias = qobject_cast<Dynamic::BiasedPlaylist*>(playlist)->bias();
    biasIndex = model->index( 0, 0, playlistIndex );
    QCOMPARE( model->index( bias.data() ), biasIndex );

    Dynamic::BiasPtr subBias = qobject_cast<Dynamic::AndBias*>(bias.data())->biases().at(0);
    subBiasIndex = model->index( 0, 0, biasIndex );
    QCOMPARE( model->index( subBias.data() ), subBiasIndex );
}

void
TestDynamicModel::testSlots()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadCurrentPlaylists();

    QSignalSpy spy1( model, SIGNAL(rowsAboutToBeRemoved(const QModelIndex&, int, int)) );
    QSignalSpy spy2( model, SIGNAL(rowsRemoved(const QModelIndex&, int, int)) );
    QSignalSpy spy3( model, SIGNAL(rowsAboutToBeInserted(const QModelIndex&, int, int)) );
    QSignalSpy spy4( model, SIGNAL(rowsInserted(const QModelIndex&, int, int)) );

    // -- removeAt with playlist
    QModelIndex playlistIndex = model->index( 1, 0 );
    QString oldName = model->data( playlistIndex ).toString();

    model->removeAt( playlistIndex );

    QCOMPARE( spy1.count(), 1 );
    QCOMPARE( spy3.count(), 0 );
    QList<QVariant> args1 = spy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<QModelIndex>() );
    QCOMPARE( args1.value(0).value<QModelIndex>(), QModelIndex() );
    QCOMPARE( args1.value(1).toInt(), 1 );
    QCOMPARE( args1.value(2).toInt(), 1 );
    QCOMPARE( spy2.count(), 1 );
    spy2.takeFirst();

    // name should be different
    playlistIndex = model->index( 1, 0 );
    QVERIFY( model->data( playlistIndex ).toString() != oldName );

    QCOMPARE( model->rowCount(), 3 );

    // -- removeAt with bias
    playlistIndex = model->index( 1, 0 );
    QModelIndex biasIndex = model->index( 0, 0, playlistIndex );
    QModelIndex subBiasIndex = model->index( 0, 0, biasIndex );
    QCOMPARE( model->rowCount( biasIndex ), 2 );

    model->removeAt( subBiasIndex );

    QCOMPARE( spy1.count(), 1 );
    QCOMPARE( spy3.count(), 0 );
    args1 = spy1.takeFirst();
    QVERIFY( args1.value(0).canConvert<QModelIndex>() );
    QCOMPARE( args1.value(0).value<QModelIndex>(), biasIndex );
    QCOMPARE( args1.value(1).toInt(), 0 );
    QCOMPARE( args1.value(2).toInt(), 0 );
    QCOMPARE( spy2.count(), 1 );
    spy2.takeFirst();

    QCOMPARE( model->rowCount( biasIndex ), 1 );
    QCOMPARE( model->rowCount(), 3 ); // only the bias was removed

    // -- cloneAt with level 2 bias
    playlistIndex = model->index( 1, 0 );

    biasIndex = model->index( 0, 0, playlistIndex );
    subBiasIndex = model->index( 0, 0, biasIndex );
    QCOMPARE( model->rowCount( biasIndex ), 1 );

    QModelIndex resultIndex = model->cloneAt(subBiasIndex);
    QCOMPARE( resultIndex.row(), 1 );
    qDebug() << "resultIndex" << resultIndex.parent() <<"should be"<<biasIndex;
    QCOMPARE( resultIndex.parent(), biasIndex );

    QCOMPARE( spy3.count(), 1 );
    args1 = spy3.takeFirst();
    QVERIFY( args1.value(0).canConvert<QModelIndex>() );
    QCOMPARE( args1.value(0).value<QModelIndex>(), biasIndex );
    QCOMPARE( args1.value(1).toInt(), 1 );
    QCOMPARE( args1.value(2).toInt(), 1 );
    QCOMPARE( spy4.count(), 1 );
    spy4.takeFirst();

    QCOMPARE( model->rowCount( biasIndex ), 2 );
    QCOMPARE( model->rowCount(), 3 ); // only the bias was cloned

    // -- newPlaylist
    QCOMPARE( spy1.count(), 0 );
    QCOMPARE( spy3.count(), 0 );

    QCOMPARE( model->rowCount(), 3 );
    resultIndex = model->newPlaylist();
    QCOMPARE( model->rowCount(), 4 );

    QCOMPARE( resultIndex.row(), 3 );
    QCOMPARE( resultIndex.parent(), QModelIndex() );

    QCOMPARE( spy1.count(), 0 );
    QCOMPARE( spy3.count(), 1 );
    args1 = spy3.takeFirst();
    QVERIFY( args1.value(0).canConvert<QModelIndex>() );
    QCOMPARE( args1.value(0).value<QModelIndex>(), QModelIndex() );
    QCOMPARE( args1.value(1).toInt(), 3 );
    QCOMPARE( args1.value(2).toInt(), 3 );
    QCOMPARE( spy4.count(), 1 );
    spy4.takeFirst();

    // -- cloneAt with playlist
    playlistIndex = model->index( 1, 0 );

    QCOMPARE( model->rowCount(), 4 );
    resultIndex = model->cloneAt(playlistIndex);
    QCOMPARE( model->rowCount(), 5 );

    QCOMPARE( resultIndex.row(), 4 );
    QCOMPARE( resultIndex.parent(), QModelIndex() );
    QCOMPARE( model->rowCount( resultIndex ), 1 );

    QCOMPARE( spy3.count(), 1 );
    args1 = spy3.takeFirst();
    QVERIFY( args1.value(0).canConvert<QModelIndex>() );
    QCOMPARE( args1.value(0).value<QModelIndex>(), QModelIndex() );
    QCOMPARE( args1.value(1).toInt(), 4 );
    QCOMPARE( args1.value(2).toInt(), 4 );
    QCOMPARE( spy4.count(), 1 );
    spy4.takeFirst();

}

void
TestDynamicModel::testSaveActive()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadCurrentPlaylists();

    // now we should have the default playlist
    // QCOMPARE( model->isActiveUnsaved(), false );
    // QCOMPARE( model->isActiveDefault(), true );
    QCOMPARE( model->activePlaylistIndex(), 0 );
    // QCOMPARE( model->defaultPlaylistIndex(), model->activePlaylistIndex() );
    QCOMPARE( model->activePlaylist()->title(), QString("Random") );

    // change the default playlist
    Dynamic::BiasedPlaylist* playlist = qobject_cast<Dynamic::BiasedPlaylist*>(model->activePlaylist());
    playlist->bias()->replace( Dynamic::BiasPtr( new Dynamic::UniqueBias() ) );

    // QCOMPARE( model->isActiveUnsaved(), true );
    // QVERIFY( model->data( model->index(0, 0) ).toString() != QString("Random") );

    // saving and loading the playlist does not change the state
    model->saveCurrentPlaylists();
    // QCOMPARE( model->isActiveUnsaved(), true );
    QCOMPARE( model->activePlaylistIndex(), 0 );

    model->loadCurrentPlaylists();
    // QCOMPARE( model->isActiveUnsaved(), true );
    QCOMPARE( model->activePlaylistIndex(), 0 );

    // model->saveActive( "New Playlist" );

    // Saved the playlist under a new name (at the end)
    QCOMPARE( model->columnCount(), 1 );
    QCOMPARE( model->rowCount(), 2 );
    QCOMPARE( model->data( model->index(model->activePlaylistIndex(), 0) ).toString(),
              QString("New Playlist") );
    // QCOMPARE( model->isActiveUnsaved(), false );
    // QCOMPARE( model->isActiveDefault(), false );
    QCOMPARE( model->activePlaylistIndex(), 1 );
    // QVERIFY( model->defaultPlaylistIndex() != model->activePlaylistIndex() );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist") );

    // Save the unmodified playlist under a new name
    // model->saveActive( "New Playlist 2" );
    QCOMPARE( model->rowCount(), 3 );
    // QCOMPARE( model->isActiveUnsaved(), false );
    // QCOMPARE( model->isActiveDefault(), false );
    QCOMPARE( model->activePlaylistIndex(), 2 );

    // change the default playlist again
    // model->setActivePlaylist( model->defaultPlaylistIndex() );
    playlist = qobject_cast<Dynamic::BiasedPlaylist*>(model->activePlaylist());
    playlist->bias()->replace( Dynamic::BiasPtr( new Dynamic::UniqueBias() ) );

    // QCOMPARE( model->isActiveUnsaved(), true );

    // Saved the playlist under an already existing
    // model->saveActive( "New Playlist" );
    QCOMPARE( model->rowCount(), 3 );
    QCOMPARE( model->data( model->index(model->activePlaylistIndex(), 0) ).toString(),
              QString("New Playlist") );
    // QCOMPARE( model->isActiveUnsaved(), false );
    // QCOMPARE( model->isActiveDefault(), false );
    QCOMPARE( model->activePlaylistIndex(), 2 );
    // QVERIFY( model->defaultPlaylistIndex() != model->activePlaylistIndex() );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist") );
}

void
TestDynamicModel::testRemoveActive()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadCurrentPlaylists();

    // create three new playlists
    // model->saveActive( "New Playlist" );
    // model->saveActive( "New Playlist 2" );
    // model->saveActive( "New Playlist 3" );

    QCOMPARE( model->rowCount(), 4 );
    // QCOMPARE( model->activePlaylist()->title(), QString("New Playlist 3") );

    // try to remove the default playlist
    // model->setActivePlaylist( model->defaultPlaylistIndex() );
    // model->removeActive();
    QCOMPARE( model->rowCount(), 4 );

    // try to remove the the second
    model->setActivePlaylist( 2 );
    // model->removeActive();
    QCOMPARE( model->rowCount(), 3 );

    model->setActivePlaylist( 1 );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist") );

    model->setActivePlaylist( 2 );
    QCOMPARE( model->activePlaylist()->title(), QString("New Playlist 3") );
}

#include "TestDynamicModel.moc"
