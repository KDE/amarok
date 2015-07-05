/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@asbest-online.de>               *
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

#include "TestXSPFPlaylist.h"

#include "config-amarok-test.h"
#include "EngineController.h"
#include "core/support/Components.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "core-impl/playlists/types/file/xspf/XSPFPlaylist.h"

#include <KStandardDirs>
#include <qtest_kde.h>
#include <ThreadWeaver/Queue>

#include <QDebug>
#include <QTest>
#include <QDir>
#include <QFile>

QTEST_KDEMAIN_CORE( TestXSPFPlaylist )

TestXSPFPlaylist::TestXSPFPlaylist()
{}

QString
TestXSPFPlaylist::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QString( AMAROK_TEST_DIR ) + '/' + relPath );
}


void TestXSPFPlaylist::initTestCase()
{
    // EngineController is used in a connection in MetaProxy::Track; avoid null sender
    // warning
    EngineController *controller = new EngineController();
    Amarok::Components::setEngineController( controller );

    qRegisterMetaType<Meta::TrackPtr>( "Meta::TrackPtr" );

    /* Collection manager needs to be instantiated in the main thread, but
     * MetaProxy::Tracks used by playlist may trigger its creation in a different thread.
     * Pre-create it explicitly */
    CollectionManager::instance();

    const QString testXspf = "data/playlists/test.xspf";
    const QUrl url = dataPath( testXspf );
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream1;

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream1.setDevice( &playlistFile1 );
    QVERIFY( playlistStream1.device() );

    qDebug() << "got playlist path: " << url.url();

    //we need to copy this laylist file to a temp dir as some of the tests we do will delete/overwrite it
    QString tempPath = KStandardDirs::locateLocal( "tmp", "test.xspf" );
    qDebug() << "got temp path: " << tempPath;
    QFile::remove( tempPath );
    QVERIFY( QFile::copy( url.toLocalFile(), tempPath ) );
    QVERIFY( QFile::exists( tempPath ) );

    m_testPlaylist1 = new Playlists::XSPFPlaylist( tempPath );
    QVERIFY( m_testPlaylist1 );
    QVERIFY( m_testPlaylist1->load( playlistStream1 ) );
}

void TestXSPFPlaylist::cleanupTestCase()
{
    QFile::remove( KStandardDirs::locateLocal( "tmp", "test.xspf" ) );
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Queue::instance()->finish();

    delete m_testPlaylist1;
    delete Amarok::Components::setEngineController( 0 );
}

void TestXSPFPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist1->name(), QString( "my playlist" ) );

    m_testPlaylist1->setName( "test" );
    QCOMPARE( m_testPlaylist1->name(), QString( "test" ) );

    m_testPlaylist1->setName( "test aäoöuüß" );
    QCOMPARE( m_testPlaylist1->name(), QString( "test aäoöuüß" ) );

    m_testPlaylist1->setName( "" );
    QCOMPARE( m_testPlaylist1->name(), QString( "" ) );
}

void TestXSPFPlaylist::prettyName()
{
    QCOMPARE( m_testPlaylist1->prettyName(), QString( "" ) );
}

void TestXSPFPlaylist::testSetAndGetTracks()
{
    Meta::TrackList tracklist = m_testPlaylist1->tracks();

    QCOMPARE( tracklist.size(), 23 );
    QCOMPARE( tracklist.at( 0 ).data()->name(), QString( "Sunset" ) );
    QCOMPARE( tracklist.at( 1 ).data()->name(), QString( "Heaven" ) );
    QCOMPARE( tracklist.at( 2 ).data()->name(), QString( "Liquid Sun" ) );
    QCOMPARE( tracklist.at( 3 ).data()->name(), QString( "Restrained Mind" ) );
    QCOMPARE( tracklist.at( 22 ).data()->name(), QString( "Trash Bag" ) );
}

void TestXSPFPlaylist::testSetAndGetTitle()
{
    QCOMPARE( m_testPlaylist1->title(), QString( "" ) );

    m_testPlaylist1->setTitle( "test" );
    QCOMPARE( m_testPlaylist1->title(), QString( "test" ) );

    m_testPlaylist1->setTitle( "test aäoöuüß" );
    QCOMPARE( m_testPlaylist1->title(), QString( "test aäoöuüß" ) );

    m_testPlaylist1->setTitle( "" );
    QCOMPARE( m_testPlaylist1->title(), QString( "" ) );
}

void TestXSPFPlaylist::testSetAndGetCreator()
{
    QCOMPARE( m_testPlaylist1->creator(), QString( "" ) );

    m_testPlaylist1->setCreator( "test" );
    QCOMPARE( m_testPlaylist1->creator(), QString( "test" ) );

    m_testPlaylist1->setCreator( "test aäoöuüß" );
    QCOMPARE( m_testPlaylist1->creator(), QString( "test aäoöuüß" ) );

    m_testPlaylist1->setCreator( "" );
    QCOMPARE( m_testPlaylist1->creator(), QString( "" ) );
}

void TestXSPFPlaylist::testSetAndGetAnnotation()
{
    QCOMPARE( m_testPlaylist1->annotation(), QString( "" ) );

    m_testPlaylist1->setAnnotation( "test" );
    QCOMPARE( m_testPlaylist1->annotation(), QString( "test" ) );

    m_testPlaylist1->setAnnotation( "test aäoöuüß" );
    QCOMPARE( m_testPlaylist1->annotation(), QString( "test aäoöuüß" ) );

    m_testPlaylist1->setAnnotation( "" );
    QCOMPARE( m_testPlaylist1->annotation(), QString( "" ) );
}

void TestXSPFPlaylist::testSetAndGetInfo()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->info().pathOrUrl(), QString( "" ) );

    testUrl = "http://amarok.kde.org";
    m_testPlaylist1->setInfo( testUrl );
    QCOMPARE( m_testPlaylist1->info().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "http://öko.de";
    m_testPlaylist1->setInfo( testUrl );
    QCOMPARE( m_testPlaylist1->info().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "";
    m_testPlaylist1->setInfo( testUrl );
    QCOMPARE( m_testPlaylist1->info().pathOrUrl(), QUrl( "" ).pathOrUrl() );
}

void TestXSPFPlaylist::testSetAndGetLocation()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->location().pathOrUrl(), QString( "" ) );

    testUrl = "http://amarok.kde.org";
    m_testPlaylist1->setLocation( testUrl );
    QCOMPARE( m_testPlaylist1->location().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "http://öko.de";
    m_testPlaylist1->setLocation( testUrl );
    QCOMPARE( m_testPlaylist1->location().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "";
    m_testPlaylist1->setLocation( testUrl );
    QCOMPARE( m_testPlaylist1->location().pathOrUrl(), QUrl( "" ).pathOrUrl() );
}

void TestXSPFPlaylist::testSetAndGetIdentifier()
{
    QCOMPARE( m_testPlaylist1->identifier(), QString( "" ) );

    m_testPlaylist1->setIdentifier( "test" );
    QCOMPARE( m_testPlaylist1->identifier(), QString( "test" ) );

    m_testPlaylist1->setIdentifier( "test aäoöuüß" );
    QCOMPARE( m_testPlaylist1->identifier(), QString( "test aäoöuüß" ) );

    m_testPlaylist1->setIdentifier( "" );
    QCOMPARE( m_testPlaylist1->identifier(), QString( "" ) );
}

void TestXSPFPlaylist::testSetAndGetImage()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->image().pathOrUrl(), QString( "" ) );

    testUrl = "http://amarok.kde.org";
    m_testPlaylist1->setImage( testUrl );
    QCOMPARE( m_testPlaylist1->image().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "http://öko.de";
    m_testPlaylist1->setImage( testUrl );
    QCOMPARE( m_testPlaylist1->image().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "";
    m_testPlaylist1->setImage( testUrl );
    QCOMPARE( m_testPlaylist1->image().pathOrUrl(), QUrl( "" ).pathOrUrl() );
}

void TestXSPFPlaylist::testSetAndGetDate()
{
    QDateTime testDateTime;
    QCOMPARE( m_testPlaylist1->date().toString(), QString( "" ) );

    testDateTime = QDateTime::fromString( "2009/08/13 13:57:18", "yyyy/MM/dd hh:mm:ss" );
    m_testPlaylist1->setDate( testDateTime );
    QCOMPARE( m_testPlaylist1->date(), testDateTime );

    testDateTime = QDateTime::fromString( "", "" );
    m_testPlaylist1->setDate( testDateTime );
    QCOMPARE( m_testPlaylist1->date(), testDateTime );
}

void TestXSPFPlaylist::testSetAndGetLicense()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->license().pathOrUrl(), QString( "" ) );

    testUrl = "http://amarok.kde.org";
    m_testPlaylist1->setLicense( testUrl );
    QCOMPARE( m_testPlaylist1->license().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "http://öko.de";
    m_testPlaylist1->setLicense( testUrl );
    QCOMPARE( m_testPlaylist1->license().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "";
    m_testPlaylist1->setLicense( testUrl );
    QCOMPARE( m_testPlaylist1->license().pathOrUrl(), QUrl( "" ).pathOrUrl() );
}

void TestXSPFPlaylist::testSetAndGetAttribution()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->attribution().size(), 0 );

    testUrl = "http://amarok.kde.org";
    m_testPlaylist1->setAttribution( testUrl );
    QCOMPARE( m_testPlaylist1->attribution().size(), 1 );
    QCOMPARE( m_testPlaylist1->attribution().at( 0 ).pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "http://öko.de";
    m_testPlaylist1->setAttribution( testUrl );
    QCOMPARE( m_testPlaylist1->attribution().size(), 2 );
    QCOMPARE( m_testPlaylist1->attribution().at( 0 ).pathOrUrl(), QUrl( testUrl ).pathOrUrl() );
    QCOMPARE( m_testPlaylist1->attribution().at( 1 ).pathOrUrl(), QUrl("http://amarok.kde.org").pathOrUrl() );

    testUrl = "http://test.com";
    m_testPlaylist1->setAttribution( testUrl, false );
    QCOMPARE( m_testPlaylist1->attribution().size(), 1 );
    QCOMPARE( m_testPlaylist1->attribution().at( 0 ).pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "";
    m_testPlaylist1->setAttribution( testUrl );
    QCOMPARE( m_testPlaylist1->attribution().size(), 1 ); // empty url won't be added, but size is 1 from last addition
}

void TestXSPFPlaylist::testSetAndGetLink()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->link().pathOrUrl(), QString( "" ) );

    testUrl = "http://amarok.kde.org";
    m_testPlaylist1->setLink( testUrl );
    QCOMPARE( m_testPlaylist1->link().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "http://öko.de";
    m_testPlaylist1->setLink( testUrl );
    QCOMPARE( m_testPlaylist1->link().pathOrUrl(), QUrl( testUrl ).pathOrUrl() );

    testUrl = "";
    m_testPlaylist1->setLink( testUrl );
    QCOMPARE( m_testPlaylist1->link().pathOrUrl(), QString( "" ) );
}

void TestXSPFPlaylist::testUidUrl()
{
    QString tempPath = KStandardDirs::locateLocal( "tmp", "test.xspf" );
    
    //we have chaged the name around so much, better reset it
    m_testPlaylist1->setName( "test" );
    QCOMPARE( m_testPlaylist1->uidUrl().pathOrUrl(), tempPath );
}

void TestXSPFPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist1->isWritable() );
}

void TestXSPFPlaylist::testSave()
{
    QVERIFY( m_testPlaylist1->save( false ) );
}
