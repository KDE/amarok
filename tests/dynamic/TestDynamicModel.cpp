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

#include "amarokconfig.h"
#include "dynamic/Bias.h"
#include "dynamic/BiasedPlaylist.h"
#include "dynamic/DynamicModel.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QByteArray>
#include <QDataStream>
#include <QMimeData>
#include <QSignalSpy>
#include <QTemporaryDir>


Q_DECLARE_METATYPE(QModelIndex);

QTemporaryDir *s_tmpDir = nullptr;   // Memory leak here now, but if it's deleted, we have a segfault

// We return a special saveLocation.
QString Amarok::saveLocation( const QString &directory )
{
    return s_tmpDir->path() + directory;
}

QTEST_GUILESS_MAIN( TestDynamicModel )

TestDynamicModel::TestDynamicModel()
{
    qRegisterMetaType<QModelIndex>();
}

void
TestDynamicModel::init()
{
    KLocalizedString::setApplicationDomain("amarok-test");
    AmarokConfig::instance("amarokrc");

    s_tmpDir = new QTemporaryDir();
    QVERIFY( s_tmpDir->isValid() );
}

void
TestDynamicModel::cleanup()
{
}

void
TestDynamicModel::testData()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadPlaylists();

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
    model->loadPlaylists();

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
    QModelIndex subBiasIndex = model->index( 1, 0, biasIndex );

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
    QCOMPARE( model->index( bias ), biasIndex );

    Dynamic::BiasPtr subBias = qobject_cast<Dynamic::AndBias*>(bias.data())->biases().at(0);
    subBiasIndex = model->index( 0, 0, biasIndex );
    QCOMPARE( model->index( subBias ), subBiasIndex );
}

void
TestDynamicModel::testSlots()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadPlaylists();

    QSignalSpy spy1( model, &Dynamic::DynamicModel::rowsAboutToBeRemoved );
    QSignalSpy spy2( model, &Dynamic::DynamicModel::rowsRemoved );
    QSignalSpy spy3( model, &Dynamic::DynamicModel::rowsAboutToBeInserted );
    QSignalSpy spy4( model, &Dynamic::DynamicModel::rowsInserted );

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
    QCOMPARE( args1.count(), 3 );
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

QModelIndex
TestDynamicModel::serializeUnserialize( const QModelIndex& index )
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    QByteArray bytes;
    QDataStream stream( &bytes, QIODevice::WriteOnly );
    model->serializeIndex( &stream, index );

    QDataStream stream2( &bytes, QIODevice::ReadOnly );
    return model->unserializeIndex( &stream2 );
}

void
TestDynamicModel::testSerializeIndex()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadPlaylists();

    QModelIndex playlistIndex = model->index( 2, 0 );
    QModelIndex biasIndex = model->index( 0, 0, playlistIndex );
    QModelIndex subBiasIndex = model->index( 0, 0, biasIndex );

    QCOMPARE( QModelIndex(), serializeUnserialize( QModelIndex() ) );
    QCOMPARE( biasIndex, serializeUnserialize( biasIndex ) );
    QCOMPARE( subBiasIndex, serializeUnserialize( subBiasIndex ) );
}

void
TestDynamicModel::testDnD()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadPlaylists();

    // -- copy a playlist
    QModelIndex playlistIndex = model->index( 2, 0 );
    QModelIndexList indexes;
    indexes << playlistIndex;
    int oldRowCount = model->rowCount();
    QString oldName = model->data( playlistIndex ).toString();
    QMimeData* data = model->mimeData( indexes );
    QVERIFY( model->dropMimeData( data, Qt::CopyAction, 0, 0, QModelIndex() ) );

    QCOMPARE( model->rowCount(), oldRowCount + 1 );
    playlistIndex = model->index( 0, 0 );
    QCOMPARE( oldName, model->data( playlistIndex ).toString() );
    delete data;

    // -- move a playlist (to the end)
    playlistIndex = model->index( 0, 0 );
    indexes.clear();
    indexes << playlistIndex;

    oldRowCount = model->rowCount();
    oldName = model->data( playlistIndex ).toString();
    data = model->mimeData( indexes );
    QVERIFY( model->dropMimeData( data, Qt::MoveAction, oldRowCount, 0, QModelIndex() ) );

    QCOMPARE( model->rowCount(), oldRowCount );
    playlistIndex = model->index( oldRowCount - 1, 0 );
    QCOMPARE( oldName, model->data( playlistIndex ).toString() );
    delete data;


    // -- copy a bias
    // TODO
//     QModelIndex biasIndex = model->index( 0, 0, playlistIndex );
//     QModelIndex subBiasIndex = model->index( 0, 0, biasIndex );
}

void
TestDynamicModel::testRemoveActive()
{
    Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();

    // load from the empty directory
    model->loadPlaylists();
    QCOMPARE( model->rowCount(), 4 );

    // -- try to remove the active playlist
    model->setActivePlaylist( 2 );
    QCOMPARE( model->activePlaylistIndex(), 2 );
    Dynamic::DynamicPlaylist* pl = model->activePlaylist();

    model->removeAt( model->index( pl ) );
    QCOMPARE( model->rowCount(), 3 );
    QVERIFY( model->activePlaylist() != pl );

    // -- now remove all playlists remaining three playlists
    model->removeAt( model->index( model->activePlaylist() ) );
    model->removeAt( model->index( model->activePlaylist() ) );
    model->removeAt( model->index( model->activePlaylist() ) );

    QCOMPARE( model->rowCount(), 0 );
    QCOMPARE( model->activePlaylist(), static_cast<Dynamic::DynamicPlaylist*>(nullptr) );
}

