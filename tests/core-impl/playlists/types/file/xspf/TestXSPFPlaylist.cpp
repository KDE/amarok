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

#include <ThreadWeaver/Queue>

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTest>


QTEST_GUILESS_MAIN( TestXSPFPlaylist )

TestXSPFPlaylist::TestXSPFPlaylist()
{}

QString
TestXSPFPlaylist::dataPath( const QString &relPath )
{
    return QDir::toNativeSeparators( QStringLiteral( AMAROK_TEST_DIR ) + QLatin1Char('/') + relPath );
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

    const QString testXspf = QStringLiteral("data/playlists/test2.xspf");
    const QUrl url = QUrl::fromLocalFile(dataPath( testXspf ));
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream1;

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream1.setDevice( &playlistFile1 );
    QVERIFY( playlistStream1.device() );

    qDebug() << "got playlist path: " << url.url();

    //we need to copy this playlist file to a temp dir as some of the tests we do will delete/overwrite it
    m_tempDir = new QTemporaryDir;;
    QVERIFY( m_tempDir->isValid() );
    QString tempPath = m_tempDir->path() + QStringLiteral("/test.xspf");
    qDebug() << "got temp path: " << tempPath;
    QVERIFY( QFile::copy( url.toLocalFile(), tempPath ) );
    QVERIFY( QFile::exists( tempPath ) );

    m_testPlaylist1 = new Playlists::XSPFPlaylist( QUrl::fromLocalFile(tempPath) );
    QVERIFY( m_testPlaylist1 );
    QVERIFY( m_testPlaylist1->load( playlistStream1 ) );
}

void TestXSPFPlaylist::cleanupTestCase()
{
    QFile::remove( QStandardPaths::locate( QStandardPaths::TempLocation, QStringLiteral("test.xspf") ) );
    // Wait for other jobs, like MetaProxys fetching meta data, to finish
    ThreadWeaver::Queue::instance()->finish();

    delete m_testPlaylist1;
    delete m_tempDir;
    delete Amarok::Components::setEngineController( nullptr );
}

void TestXSPFPlaylist::testSetAndGetName()
{
    QCOMPARE( m_testPlaylist1->name(), QStringLiteral( "my playlist" ) );

    m_testPlaylist1->setName( QStringLiteral("test") );
    QCOMPARE( m_testPlaylist1->name(), QStringLiteral( "test" ) );

    m_testPlaylist1->setName( QStringLiteral("test aäoöuüß") );
    QCOMPARE( m_testPlaylist1->name(), QStringLiteral( "test aäoöuüß" ) );

    m_testPlaylist1->setName( QStringLiteral("") );
    QCOMPARE( m_testPlaylist1->name(), QStringLiteral( "" ) );
}

void TestXSPFPlaylist::prettyName()
{
    QCOMPARE( m_testPlaylist1->prettyName(), QStringLiteral( "" ) );
}

void TestXSPFPlaylist::testSetAndGetTracks()
{
    Meta::TrackList tracklist = m_testPlaylist1->tracks();

    QCOMPARE( tracklist.size(), 24 );
    QCOMPARE( tracklist.at( 0 )->name(), QStringLiteral( "Sunset" ) );
    QCOMPARE( tracklist.at( 1 )->name(), QStringLiteral( "Heaven" ) );
    QCOMPARE( tracklist.at( 2 )->name(), QStringLiteral( "Liquid Sun" ) );
    QCOMPARE( tracklist.at( 3 )->name(), QStringLiteral( "Restrained Mind" ) );
    QCOMPARE( tracklist.at( 22 )->name(), QStringLiteral( "Trash Bag" ) );
    QCOMPARE( tracklist.at( 23 )->name(), QStringLiteral( "Test träck" ) );
}

void TestXSPFPlaylist::testSetAndGetTitle()
{
    QCOMPARE( m_testPlaylist1->title(), QStringLiteral( "" ) );

    m_testPlaylist1->setTitle( QStringLiteral("test") );
    QCOMPARE( m_testPlaylist1->title(), QStringLiteral( "test" ) );

    m_testPlaylist1->setTitle( QStringLiteral("test aäoöuüß") );
    QCOMPARE( m_testPlaylist1->title(), QStringLiteral( "test aäoöuüß" ) );

    m_testPlaylist1->setTitle( QStringLiteral("") );
    QCOMPARE( m_testPlaylist1->title(), QStringLiteral( "" ) );
}

void TestXSPFPlaylist::testSetAndGetCreator()
{
    QCOMPARE( m_testPlaylist1->creator(), QStringLiteral( "" ) );

    m_testPlaylist1->setCreator( QStringLiteral("test") );
    QCOMPARE( m_testPlaylist1->creator(), QStringLiteral( "test" ) );

    m_testPlaylist1->setCreator( QStringLiteral("test aäoöuüß") );
    QCOMPARE( m_testPlaylist1->creator(), QStringLiteral( "test aäoöuüß" ) );

    m_testPlaylist1->setCreator( QStringLiteral("") );
    QCOMPARE( m_testPlaylist1->creator(), QStringLiteral( "" ) );
}

void TestXSPFPlaylist::testSetAndGetAnnotation()
{
    QCOMPARE( m_testPlaylist1->annotation(), QStringLiteral( "" ) );

    m_testPlaylist1->setAnnotation( QStringLiteral("test") );
    QCOMPARE( m_testPlaylist1->annotation(), QStringLiteral( "test" ) );

    m_testPlaylist1->setAnnotation( QStringLiteral("test aäoöuüß") );
    QCOMPARE( m_testPlaylist1->annotation(), QStringLiteral( "test aäoöuüß" ) );

    m_testPlaylist1->setAnnotation( QStringLiteral("") );
    QCOMPARE( m_testPlaylist1->annotation(), QStringLiteral( "" ) );
}

void TestXSPFPlaylist::testSetAndGetInfo()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->info(), QUrl( QStringLiteral("") ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://amarok.kde.org"));
    m_testPlaylist1->setInfo( testUrl );
    QCOMPARE( m_testPlaylist1->info(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://öko.de"));
    m_testPlaylist1->setInfo( testUrl );
    QCOMPARE( m_testPlaylist1->info(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral(""));
    m_testPlaylist1->setInfo( testUrl );
    QCOMPARE( m_testPlaylist1->info(), QUrl( QStringLiteral("") ) );
}

void TestXSPFPlaylist::testSetAndGetLocation()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->location(), QUrl( QStringLiteral("") ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://amarok.kde.org"));
    m_testPlaylist1->setLocation( testUrl );
    QCOMPARE( m_testPlaylist1->location(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://öko.de"));
    m_testPlaylist1->setLocation( testUrl );
    QCOMPARE( m_testPlaylist1->location(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral(""));
    m_testPlaylist1->setLocation( testUrl );
    QCOMPARE( m_testPlaylist1->location(), QUrl( QStringLiteral("") ) );
}

void TestXSPFPlaylist::testSetAndGetIdentifier()
{
    QCOMPARE( m_testPlaylist1->identifier(), QStringLiteral( "" ) );

    m_testPlaylist1->setIdentifier( QStringLiteral("test") );
    QCOMPARE( m_testPlaylist1->identifier(), QStringLiteral( "test" ) );

    m_testPlaylist1->setIdentifier( QStringLiteral("test aäoöuüß") );
    QCOMPARE( m_testPlaylist1->identifier(), QStringLiteral( "test aäoöuüß" ) );

    m_testPlaylist1->setIdentifier( QStringLiteral("") );
    QCOMPARE( m_testPlaylist1->identifier(), QStringLiteral( "" ) );
}

void TestXSPFPlaylist::testSetAndGetImage()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->image(), QUrl( QStringLiteral("") ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://amarok.kde.org"));
    m_testPlaylist1->setImage( testUrl );
    QCOMPARE( m_testPlaylist1->image(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://öko.de"));
    m_testPlaylist1->setImage( testUrl );
    QCOMPARE( m_testPlaylist1->image(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral(""));
    m_testPlaylist1->setImage( testUrl );
    QCOMPARE( m_testPlaylist1->image(), QUrl( QStringLiteral("") ) );
}

void TestXSPFPlaylist::testSetAndGetDate()
{
    QDateTime testDateTime;
    QCOMPARE( m_testPlaylist1->date().toString(), QStringLiteral( "" ) );

    testDateTime = QDateTime::fromString( QStringLiteral("2009/08/13 13:57:18"), QStringLiteral("yyyy/MM/dd hh:mm:ss") );
    m_testPlaylist1->setDate( testDateTime );
    QCOMPARE( m_testPlaylist1->date(), testDateTime );

    testDateTime = QDateTime::fromString( QStringLiteral(""), QStringLiteral("") );
    m_testPlaylist1->setDate( testDateTime );
    QCOMPARE( m_testPlaylist1->date(), testDateTime );
}

void TestXSPFPlaylist::testSetAndGetLicense()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->license(), QUrl( QStringLiteral("") ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://amarok.kde.org"));
    m_testPlaylist1->setLicense( testUrl );
    QCOMPARE( m_testPlaylist1->license(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://öko.de"));
    m_testPlaylist1->setLicense( testUrl );
    QCOMPARE( m_testPlaylist1->license(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral(""));
    m_testPlaylist1->setLicense( testUrl );
    QCOMPARE( m_testPlaylist1->license(), QUrl( QStringLiteral("") ) );
}

void TestXSPFPlaylist::testSetAndGetAttribution()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->attribution().size(), 0 );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://amarok.kde.org"));
    m_testPlaylist1->setAttribution( testUrl );
    QCOMPARE( m_testPlaylist1->attribution().size(), 1 );
    QCOMPARE( m_testPlaylist1->attribution().at( 0 ), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://öko.de"));
    m_testPlaylist1->setAttribution( testUrl );
    QCOMPARE( m_testPlaylist1->attribution().size(), 2 );
    QCOMPARE( m_testPlaylist1->attribution().at( 0 ), QUrl( testUrl ) );
    QCOMPARE( m_testPlaylist1->attribution().at( 1 ), QUrl(QStringLiteral("http://amarok.kde.org")) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://test.com"));
    m_testPlaylist1->setAttribution( testUrl, false );
    QCOMPARE( m_testPlaylist1->attribution().size(), 1 );
    QCOMPARE( m_testPlaylist1->attribution().at( 0 ), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral(""));
    m_testPlaylist1->setAttribution( testUrl );
    QCOMPARE( m_testPlaylist1->attribution().size(), 1 ); // empty url won't be added, but size is 1 from last addition
}

void TestXSPFPlaylist::testSetAndGetLink()
{
    QUrl testUrl;

    QCOMPARE( m_testPlaylist1->link(), QUrl( QStringLiteral("") ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://amarok.kde.org"));
    m_testPlaylist1->setLink( testUrl );
    QCOMPARE( m_testPlaylist1->link(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral("http://öko.de"));
    m_testPlaylist1->setLink( testUrl );
    QCOMPARE( m_testPlaylist1->link(), QUrl( testUrl ) );

    testUrl = QUrl::fromUserInput(QStringLiteral(""));
    m_testPlaylist1->setLink( testUrl );
    QCOMPARE( m_testPlaylist1->link(), QUrl( QStringLiteral("") ) );
}

void TestXSPFPlaylist::testUidUrl()
{
    QString tempPath = m_tempDir->path() + QStringLiteral("/test.xspf");

    //we have changed the name around so much, better reset it
    m_testPlaylist1->setName( QStringLiteral("test") );
    QCOMPARE( m_testPlaylist1->uidUrl().toLocalFile(), tempPath );
}

void TestXSPFPlaylist::testIsWritable()
{
    QVERIFY( m_testPlaylist1->isWritable() );
}

void TestXSPFPlaylist::testSave()
{
    QVERIFY( m_testPlaylist1->save( false ) );
}

void TestXSPFPlaylist::testSaveAndReload()
{
    QVERIFY( m_testPlaylist1->save( false ) );

    Meta::TrackList tracklist = m_testPlaylist1->tracks();
    const QString testTrack1Url = tracklist.at( 1 )->uidUrl();
    const QString testTrack2Url = tracklist.at( 23 )->uidUrl();

    const QUrl url = m_testPlaylist1->uidUrl();
    QFile playlistFile1( url.toLocalFile() );
    QTextStream playlistStream;

    QVERIFY( playlistFile1.open( QFile::ReadOnly ) );
    playlistStream.setDevice( &playlistFile1 );
    QVERIFY( playlistStream.device() );

    delete m_testPlaylist1;
    m_testPlaylist1 = new Playlists::XSPFPlaylist( url );
    QVERIFY( m_testPlaylist1 );
    QVERIFY( m_testPlaylist1->load( playlistStream ) );
    playlistFile1.close();

    tracklist = m_testPlaylist1->tracks();

    QCOMPARE( tracklist.size(), 24 );
    QCOMPARE( tracklist.at( 0 )->name(), QStringLiteral( "Sunset" ) );
    QCOMPARE( tracklist.at( 23 )->name(), QStringLiteral( "Test träck" ) );
    QCOMPARE( tracklist.at( 1 )->uidUrl(), testTrack1Url );
    QCOMPARE( tracklist.at( 23 )->uidUrl(), testTrack2Url );
}
