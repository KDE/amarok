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

#include <QTest>


QTEST_MAIN( TestFastForwardImporter )

using namespace StatSyncing;

ProviderPtr
TestFastForwardImporter::getProvider()
{
    QVariantMap cfg = FastForwardConfigWidget( QVariantMap() ).config();
    cfg.insert( QStringLiteral("dbDriver"), QStringLiteral("QSQLITE") );
    cfg.insert( QStringLiteral("dbPath"), QString( QCoreApplication::applicationDirPath() +
                          QStringLiteral("/../tests/importers_files/collection.db") ) );

    return ProviderPtr( new FastForwardProvider( cfg, nullptr ) );
}

ProviderPtr
TestFastForwardImporter::getWritableProvider()
{
    QDir base( QCoreApplication::applicationDirPath() );
    base.mkpath( QStringLiteral("importers_tmp") );

    const QString dst = base.filePath( QStringLiteral("importers_tmp/collection.db") );
    QFile( dst ).remove();
    QFile( base.filePath( QStringLiteral("../tests/importers_files/collection.db") ) ).copy( dst );

    QVariantMap cfg = FastForwardConfigWidget( QVariantMap() ).config();
    cfg.insert( QStringLiteral("dbDriver"), QStringLiteral("QSQLITE") );
    cfg.insert( QStringLiteral("dbPath"), dst );

    return ProviderPtr( new FastForwardProvider( cfg, nullptr ) );
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
    KLocalizedString::setApplicationDomain("amarok-test");
    m_cfg = FastForwardConfigWidget( QVariantMap() ).config();
}

void
TestFastForwardImporter::configWidgetShouldOnlyShowFieldsRelevantToConnection()
{
    FastForwardConfigWidget widget( m_cfg );

    const QList<QWidget*> remoteConfigWidgets = QList<QWidget*>()
            << widget.m_databaseName << widget.m_hostname << widget.m_port
            << widget.m_password << widget.m_username;

    widget.m_connectionType->setCurrentIndex( FastForwardConfigWidget::QSQLITE );
    QVERIFY( !widget.m_databaseLocation->isHidden() );
    for( auto w : remoteConfigWidgets )
        QVERIFY( w->isHidden() );

    widget.m_connectionType->setCurrentIndex( FastForwardConfigWidget::QMYSQL );
    QVERIFY( widget.m_databaseLocation->isHidden() );
    for( auto w : remoteConfigWidgets )
        QVERIFY( !w->isHidden() );

    widget.m_connectionType->setCurrentIndex( FastForwardConfigWidget::QPSQL );
    QVERIFY( widget.m_databaseLocation->isHidden() );
    for( auto w : remoteConfigWidgets )
        QVERIFY( !w->isHidden() );
}

void
TestFastForwardImporter::configWidgetShouldSetDriverNameAsConfigResult()
{
    FastForwardConfigWidget widget( m_cfg );

    widget.m_connectionType->setCurrentIndex( FastForwardConfigWidget::QSQLITE );
    QCOMPARE( widget.config().value( QStringLiteral("dbDriver") ).toString(),
              QStringLiteral( "QSQLITE" ) );

    widget.m_connectionType->setCurrentIndex( FastForwardConfigWidget::QMYSQL );
    QCOMPARE( widget.config().value( QStringLiteral("dbDriver") ).toString(),
              QStringLiteral( "QMYSQL" ) );

    widget.m_connectionType->setCurrentIndex( FastForwardConfigWidget::QPSQL );
    QCOMPARE( widget.config().value( QStringLiteral("dbDriver") ).toString(),
              QStringLiteral( "QPSQL" ) );
}

void
TestFastForwardImporter::configWidgetShouldShowSqliteAsDefault()
{
    FastForwardConfigWidget widget( m_cfg );
    QCOMPARE( widget.m_connectionType->currentIndex(),
              static_cast<int>( FastForwardConfigWidget::QSQLITE ) );
}

void
TestFastForwardImporter::configWidgetShouldNotBreakOnNonsenseInitialValues()
{
    m_cfg.insert( QStringLiteral("dbDriver"), 19 );
    m_cfg.insert( QStringLiteral("dbName"), QColor( Qt::red ) );
    m_cfg.insert( QStringLiteral("dbPort"), QStringLiteral("nonsensePort") );

    FastForwardConfigWidget widget( m_cfg );
    QVERIFY( !widget.m_databaseName->text().isEmpty() );

    const QList<QString> validDrivers = QList<QString>()
            << QStringLiteral("QSQLITE") << QStringLiteral("QMYSQL") << QStringLiteral("QPSQL");

    QVERIFY( validDrivers.contains( widget.config().value( QStringLiteral("dbDriver") ).toString() ) );
}

void
TestFastForwardImporter::configWidgetShouldReadSavedConfig()
{
    m_cfg.insert( QStringLiteral("dbDriver"), QStringLiteral("QPSQL") );
    m_cfg.insert( QStringLiteral("dbName"), QStringLiteral("MyName") );
    m_cfg.insert( QStringLiteral("dbPort"), 19 );
    m_cfg.insert( QStringLiteral("name"), QStringLiteral("theName") );

    FastForwardConfigWidget widget( m_cfg );

    QCOMPARE( widget.m_connectionType->currentIndex(),
              static_cast<int>( FastForwardConfigWidget::QPSQL ) );

    QCOMPARE( widget.m_databaseName->text(), QStringLiteral( "MyName" ) );
    QCOMPARE( widget.m_port->value(), 19 );
    QCOMPARE( widget.m_targetName->text(), QStringLiteral( "theName" ) );
}

void
TestFastForwardImporter::providerShouldHandleNonexistentDbFile()
{
    m_cfg.insert( QStringLiteral("dbPath"), QStringLiteral("/Im/sure/this/wont/exist") );

    FastForwardProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestFastForwardImporter::providerShouldHandleInvalidDbFile()
{
    m_cfg.insert( QStringLiteral("dbPath"), QApplication::applicationFilePath() );

    FastForwardProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestFastForwardImporter::providerShouldHandleExternalConnectionError()
{
    m_cfg.insert( QStringLiteral("dbDriver"), QStringLiteral("QMYSQL") );
    m_cfg.insert( QStringLiteral("dbHost"), QStringLiteral("I hope this isn't a valid hostname") );

    FastForwardProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestFastForwardImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( QStringLiteral("dbDriver"), 19 );
    m_cfg.insert( QStringLiteral("dbName"), QColor( Qt::red ) );
    m_cfg.insert( QStringLiteral("dbPort"), QStringLiteral("nonsensePort") );

    FastForwardProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

