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

#include "MetaValues.h"
#include "importers/fastforward/FastForwardConfigWidget.h"
#include "importers/fastforward/FastForwardProvider.h"

#include <QApplication>

#include <qtest_kde.h>

QTEST_KDEMAIN( TestFastForwardImporter, GUI )

using namespace StatSyncing;

ProviderPtr
TestFastForwardImporter::getProvider()
{
    QVariantMap cfg;
    cfg.insert( "dbDriver", "QSQLITE" );
    cfg.insert( "dbPath", QCoreApplication::applicationDirPath() +
                          "/importers_files/collection.db" );

    return ProviderPtr(
                new FastForwardProvider( FastForwardConfigWidget( cfg ).config(), 0 ) );
}

qint64
TestFastForwardImporter::reliableStatistics() const
{
    return Meta::valFirstPlayed | Meta::valLastPlayed | Meta::valRating
            | Meta::valPlaycount | Meta::valLabel;
}

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
    remoteConfigWidgets << m_configWidget->m_databaseName << m_configWidget->m_hostname
                        << m_configWidget->m_port << m_configWidget->m_password
                        << m_configWidget->m_username;

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
    QCOMPARE( m_configWidget->config().value( "dbDriver" ).toString(),
              QString( "QSQLITE" ) );

    m_configWidget->m_connectionType->setCurrentItem( "MySQL" );
    QCOMPARE( m_configWidget->config().value( "dbDriver" ).toString(),
              QString( "QMYSQL" ) );

    m_configWidget->m_connectionType->setCurrentItem( "PostgreSQL" );
    QCOMPARE( m_configWidget->config().value( "dbDriver" ).toString(),
              QString( "QPSQL" ) );
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
    cfg.insert( "dbDriver", 19 );
    cfg.insert( "dbName", QColor( Qt::red ) );
    cfg.insert( "dbPort", "nonsensePort" );

    QScopedPointer<FastForwardConfigWidget> widget( new FastForwardConfigWidget( cfg ) );

    QVERIFY( !widget->m_databaseName->text().isEmpty() );

    const QList<QString> validDrivers =
            QList<QString>() << "QSQLITE" << "QMYSQL" << "QPSQL";

    QVERIFY( validDrivers.contains( widget->config()["dbDriver"].toString() ) );
}

void
TestFastForwardImporter::configWidgetShouldReadSavedConfig()
{
    QVariantMap cfg;
    cfg.insert( "dbDriver", "QPSQL" );
    cfg.insert( "dbName", "MyName" );
    cfg.insert( "dbPort", 19 );
    cfg.insert( "name", "theName" );

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
    cfg.insert( "dbDriver", 19 );
    cfg.insert( "dbName", QColor( Qt::red ) );
    cfg.insert( "dbPort", "nonsensePort" );

    FastForwardProvider provider( cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

#include "TestFastForwardImporter.moc"
