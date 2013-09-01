/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "TestFastForwardImporter.h"

#include "importers/fastforward/FastForwardConfigWidget.h"
#include "importers/fastforward/FastForwardProvider.h"

#include <QApplication>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestFastForwardImporter, GUI )

using namespace StatSyncing;

void
TestFastForwardImporter::init()
{
    m_configWidget = new FastForwardConfigWidget( QVariantMap() );
}

void
TestFastForwardImporter::cleanup()
{
    delete m_configWidget;
    m_configWidget = 0;
}

void
TestFastForwardImporter::configWidgetShouldOnlyShowFieldsRelevantToConnection()
{
    QList<QWidget*> remoteConfigWidgets;
    remoteConfigWidgets << m_configWidget->m_databaseName << m_configWidget->m_hostname << m_configWidget->m_port
                        << m_configWidget->m_password << m_configWidget->m_username;

    m_configWidget->m_connectionType->setCurrentItem( "SQLite" );
    QVERIFY( !m_configWidget->m_databaseLocation->isHidden() );
    foreach( QWidget *w, remoteConfigWidgets )
        QVERIFY( w->isHidden() );

    m_configWidget->m_connectionType->setCurrentItem( "MySQL" );
    QVERIFY( m_configWidget->m_databaseLocation->isHidden() );
    foreach( QWidget *w, remoteConfigWidgets )
        QVERIFY( !w->isHidden() );

    m_configWidget->m_connectionType->setCurrentItem( "PostgreSQL" );
    QVERIFY( m_configWidget->m_databaseLocation->isHidden() );
    foreach( QWidget *w, remoteConfigWidgets )
        QVERIFY( !w->isHidden() );
}

void
TestFastForwardImporter::configWidgetShouldSetDriverNameAsConfigResult()
{
    m_configWidget->m_connectionType->setCurrentItem( "SQLite" );
    QCOMPARE( m_configWidget->config().value( "dbDriver" ).toString(), QString( "QSQLITE" ) );

    m_configWidget->m_connectionType->setCurrentItem( "MySQL" );
    QCOMPARE( m_configWidget->config().value( "dbDriver" ).toString(), QString( "QMYSQL" ) );

    m_configWidget->m_connectionType->setCurrentItem( "PostgreSQL" );
    QCOMPARE( m_configWidget->config().value( "dbDriver" ).toString(), QString( "QPSQL" ) );
}

void
TestFastForwardImporter::configWidgetShouldShowSqliteAsDefault()
{
    QCOMPARE( m_configWidget->m_connectionType->currentText(), QString( "SQLite" ) );
}

void
TestFastForwardImporter::configWidgetShouldNotBreakOnNonsenseInitialValues()
{
    QVariantMap cfg;
    cfg["dbDriver"] = 19;
    cfg["dbName"] = QColor( Qt::red );
    cfg["dbPort"] = QString( "nonsensePort" );

    QScopedPointer<FastForwardConfigWidget> widget( new FastForwardConfigWidget( cfg ) );

    QVERIFY( !widget->m_databaseName->text().isEmpty() );

    QList<QString> validDrivers = QList<QString>() << "QSQLITE" << "QMYSQL" << "QPSQL";
    QVERIFY( validDrivers.contains( widget->config()["dbDriver"].toString() ) );
}

void
TestFastForwardImporter::configWidgetShouldReadSavedConfig()
{
    QVariantMap cfg;
    cfg["dbDriver"] = QString( "QPSQL" );
    cfg["dbName"] = QString( "MyName" );
    cfg["dbPort"] = 19;
    cfg["name"] = QString( "theName" );

    QScopedPointer<FastForwardConfigWidget> widget( new FastForwardConfigWidget( cfg ) );

    QCOMPARE( widget->m_connectionType->currentText(), QString( "PostgreSQL" ) );
    QCOMPARE( widget->m_databaseName->text(), QString( "MyName" ) );
    QCOMPARE( widget->m_port->value(), 19 );
    QCOMPARE( widget->m_targetName->text(), QString( "theName" ) );
}

void
TestFastForwardImporter::providerShouldHandleNonexistentDbFile()
{
    m_configWidget->m_databaseLocation->setText( "/Im/sure/this/wont/exist" );

    FastForwardProvider provider( m_configWidget->config(), 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestFastForwardImporter::providerShouldHandleInvalidDbFile()
{
    m_configWidget->m_databaseLocation->setText( QApplication::applicationFilePath() );

    FastForwardProvider provider( m_configWidget->config(), 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestFastForwardImporter::providerShouldHandleExternalConnectionError()
{
    m_configWidget->m_databaseName->setText( "MySQL" );
    m_configWidget->m_databaseLocation->setText( "I hope this isn't a valid hostname" );

    FastForwardProvider provider( m_configWidget->config(), 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestFastForwardImporter::providerShouldHandleErroneousConfigValues()
{
    QVariantMap cfg;
    cfg["dbDriver"] = 19;
    cfg["dbName"] = QColor( Qt::red );
    cfg["dbPort"] = QString( "nonsensePort" );

    FastForwardProvider provider( cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestFastForwardImporter::providerShouldHandleNonexistentArtist()
{
    m_configWidget->m_databaseLocation->setText( QCoreApplication::applicationDirPath() + "/importers_files/collection.db" );

    FastForwardProvider provider( m_configWidget->config(), 0 );
    QVERIFY( !provider.artists().isEmpty() );
    QVERIFY( provider.artistTracks( "TheresNoSuchArtistHopefully" ).isEmpty() );
}

void
TestFastForwardImporter::artistsShouldReturnExistingArtists()
{
    m_configWidget->m_databaseLocation->setText( QCoreApplication::applicationDirPath() + "/importers_files/collection.db" );
    FastForwardProvider provider( m_configWidget->config(), 0 );

    QVERIFY( !provider.artists().isEmpty() );
    QCOMPARE( *provider.artists().begin(), QString( "Metallica" ) );
}

void
TestFastForwardImporter::artistTracksShouldReturnPopulatedTracks_data()
{
    QTest::addColumn<QString>   ( "album" );
    QTest::addColumn<QString>   ( "artist" );
    QTest::addColumn<QString>   ( "composer" );
    QTest::addColumn<QDateTime> ( "firstPlayed" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "Enter Sandman" ) << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372416489 ) << QDateTime::fromTime_t( 1372418184 ) << 14 << 9;
    QTest::newRow( "Sad but True" ) << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417027 ) << QDateTime::fromTime_t( 1372417063 ) << 7 << 5;
    QTest::newRow( "Holier Than Thou" ) << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417028 ) << QDateTime::fromTime_t( 1372417055 ) << 5 << 6;
    QTest::newRow( "The Unforgiven" ) << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372415850 ) << QDateTime::fromTime_t( 1372419217 ) << 5 << 10;
    QTest::newRow( "Wherever I May Roam" ) << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417030 ) << QDateTime::fromTime_t( 1372420415 ) << 15 << 5;
    QTest::newRow( "Don't Tread on Me" ) << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417031 ) << QDateTime::fromTime_t( 1372419457 ) << 4 << 4;
    QTest::newRow( "Through the Never" ) << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417032 ) << QDateTime::fromTime_t( 1372419702 ) << 20 << 7;
    QTest::newRow( "Nothing Else Matters" ) << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372415314 ) << QDateTime::fromTime_t( 1372418573 ) << 17 << 10;
    QTest::newRow( "Of Wolf and Man" ) << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372416881 ) << QDateTime::fromTime_t( 1372418830 ) << 2 << 8;
    QTest::newRow( "The God That Failed" ) << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417035 ) << QDateTime::fromTime_t( 1372420011 ) << 15 << 2;
    QTest::newRow( "My Friend of Misery" ) << "Metallica" << "Metallica" << "James Hetfield, Kirk Hammett, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417036 ) << QDateTime::fromTime_t( 1372417060 ) << 6 << 1;
    QTest::newRow( "The Struggle Within" ) << "Metallica" << "Metallica" << "James Hetfield, Lars Ulrich"
            << QDateTime::fromTime_t( 1372417042 ) << QDateTime::fromTime_t( 1372417061 ) << 18 << 3;
}

void
TestFastForwardImporter::artistTracksShouldReturnPopulatedTracks()
{
    m_configWidget->m_databaseLocation->setText( QCoreApplication::applicationDirPath() + "/importers_files/collection.db" );
    FastForwardProvider provider( m_configWidget->config(), 0 );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider.artistTracks(  "Metallica" ) )
        trackForName.insert( track->name(), track );

    QCOMPARE( trackForName.size(), 12 );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->album(), "album" );
    QTEST( track->artist(), "artist" );
    QTEST( track->composer(), "composer" );
    QTEST( track->firstPlayed(), "firstPlayed" );
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}

void
TestFastForwardImporter::artistTracksShouldHandleNonexistentStatistics_data()
{
    QTest::addColumn<QDateTime> ( "firstPlayed" );
    QTest::addColumn<QDateTime> ( "lastPlayed" );
    QTest::addColumn<int>       ( "playCount" );
    QTest::addColumn<int>       ( "rating" );

    QTest::newRow( "Enter Sandman" ) << QDateTime::fromTime_t( 1372416489 ) << QDateTime() << 0 << 9;
    QTest::newRow( "Sad but True" ) << QDateTime::fromTime_t( 1372417027 ) << QDateTime::fromTime_t( 1372417063 ) << 7 << 5;
    QTest::newRow( "Holier Than Thou" ) << QDateTime() << QDateTime() << 0 << 0;
    QTest::newRow( "The Unforgiven" ) << QDateTime::fromTime_t( 1372415850 ) << QDateTime::fromTime_t( 1372419217 ) << 5 << 10;
    QTest::newRow( "Wherever I May Roam" ) << QDateTime::fromTime_t( 1372417030 ) << QDateTime::fromTime_t( 1372420415 ) << 15 << 5;
    QTest::newRow( "Don't Tread on Me" ) << QDateTime() << QDateTime::fromTime_t( 1372419457 ) << 0 << 4;
    QTest::newRow( "Through the Never" ) << QDateTime::fromTime_t( 1372417032 ) << QDateTime::fromTime_t( 1372419702 ) << 0 << 0;
    QTest::newRow( "Nothing Else Matters" ) << QDateTime::fromTime_t( 1372415314 ) << QDateTime() << 17 << 0;
    QTest::newRow( "Of Wolf and Man" ) << QDateTime::fromTime_t( 1372416881 ) << QDateTime::fromTime_t( 1372418830 ) << 2 << 0;
    QTest::newRow( "The God That Failed" ) << QDateTime::fromTime_t( 1372417035 ) << QDateTime::fromTime_t( 1372420011 ) << 15 << 0;
    QTest::newRow( "My Friend of Misery" ) << QDateTime() << QDateTime() << 0 << 0;
    QTest::newRow( "The Struggle Within" ) << QDateTime() << QDateTime::fromTime_t( 1372417061 ) << 18 << 3;
}

void
TestFastForwardImporter::artistTracksShouldHandleNonexistentStatistics()
{
    m_configWidget->m_databaseLocation->setText( QCoreApplication::applicationDirPath() + "/importers_files/fastforward-no-statistics.db" );
    FastForwardProvider provider( m_configWidget->config(), 0 );

    QMap<QString, TrackPtr> trackForName;
    foreach( const TrackPtr &track, provider.artistTracks(  "Metallica" ) )
        trackForName.insert( track->name(), track );

    const QString name( QTest::currentDataTag() );
    QVERIFY( trackForName.contains( name ) );

    const TrackPtr &track = trackForName[name];
    QTEST( track->firstPlayed(), "firstPlayed" );
    QTEST( track->lastPlayed(), "lastPlayed" );
    QTEST( track->playCount(), "playCount" );
    QTEST( track->rating(), "rating" );
}

#include "TestFastForwardImporter.moc"
