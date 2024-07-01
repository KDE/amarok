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

#include "TestAmarokImporter.h"

#include "MetaValues.h"
#include "importers/amarok/AmarokConfigWidget.h"
#include "importers/amarok/AmarokManager.h"
#include "importers/amarok/AmarokProvider.h"

#include <KLocalizedString>

#include <QProcess>
#include <QTest>


Q_DECLARE_METATYPE( QProcess::ProcessError )
QTEST_MAIN( TestAmarokImporter )

using namespace StatSyncing;

ProviderPtr dbProvider = nullptr;
ProviderPtr writableDbProvider = nullptr;

TestAmarokImporter::~TestAmarokImporter()
{
    dbProvider = nullptr;
    writableDbProvider = nullptr;
}

ProviderPtr
TestAmarokImporter::getProvider()
{
    if(dbProvider)
        return dbProvider;
    QVariantMap cfg = AmarokConfigWidget( QVariantMap() ).config();
    cfg.insert( QStringLiteral("name"), QStringLiteral("Amarok2Test") );
    cfg.insert( QStringLiteral("embedded"), true );
    cfg.insert( QStringLiteral("dbPath"), QString( QCoreApplication::applicationDirPath() +
                          QStringLiteral("/../tests/importers_files/amarok2_mysqle") ) );

    return dbProvider = ProviderPtr( new AmarokProvider( cfg, nullptr ) );
}

ProviderPtr
TestAmarokImporter::getWritableProvider()
{
    if(writableDbProvider)
        return writableDbProvider;
    QDir base( QCoreApplication::applicationDirPath() );
    QDir files( base.filePath( QStringLiteral("../tests/importers_files") ) );
    QDir tmp( base.filePath( QStringLiteral("importers_tmp") ) );

    QList<QString> dirs = QList<QString>() << QStringLiteral("amarok2_mysqle") << QStringLiteral("amarok2_mysqle/amarok");
    for( auto const &subdir : dirs )
    {
        tmp.mkpath( subdir );

        QDir src( files.filePath( subdir ) );
        QDir dst( tmp.filePath( subdir ) );

        for( auto const &filename : src.entryList( QStringList(), QDir::Files ) )
        {
            QFile( dst.filePath( filename ) ).remove();
            QFile( src.filePath( filename ) ).copy( dst.filePath( filename ) );
        }
    }

    QVariantMap cfg = AmarokConfigWidget( QVariantMap() ).config();
    cfg.insert( QStringLiteral("name"), QStringLiteral("Amarok2Test") );
    cfg.insert( QStringLiteral("embedded"), true );
    cfg.insert( QStringLiteral("dbPath"), tmp.filePath( QStringLiteral("amarok2_mysqle") ) );

    return writableDbProvider = ProviderPtr( new AmarokProvider( cfg, nullptr ) );
}

qint64
TestAmarokImporter::reliableStatistics() const
{
    return Meta::valFirstPlayed | Meta::valLastPlayed | Meta::valRating
            | Meta::valPlaycount | Meta::valLabel;
}

void
TestAmarokImporter::initTestCase()
{
    qRegisterMetaType<QProcess::ProcessError>();
}

void
TestAmarokImporter::init()
{
    KLocalizedString::setApplicationDomain("amarok-test");
    m_cfg = AmarokConfigWidget( QVariantMap() ).config();
    m_cfg.insert( QStringLiteral("embedded"), true );
}

void
TestAmarokImporter::configWidgetShouldOnlyShowFieldsRelevantToConnection()
{
    AmarokConfigWidget widget( m_cfg );

    const QList<QWidget*> remoteConfigWidgets = QList<QWidget*>()
            << widget.m_databaseName << widget.m_hostname << widget.m_port
            << widget.m_password << widget.m_username;

    widget.m_connectionType->setCurrentIndex( AmarokConfigWidget::Embedded );
    QVERIFY( !widget.m_databaseLocation->isHidden() );
    for( auto w : remoteConfigWidgets )
        QVERIFY( w->isHidden() );

    widget.m_connectionType->setCurrentIndex( AmarokConfigWidget::External );
    QVERIFY( widget.m_databaseLocation->isHidden() );
    for( auto w : remoteConfigWidgets )
        QVERIFY( !w->isHidden() );
}

void
TestAmarokImporter::configWidgetShouldNotSetDriver()
{
    AmarokConfigWidget widget( m_cfg );

    widget.m_connectionType->setCurrentIndex( AmarokConfigWidget::Embedded );
    QVERIFY( !widget.config().contains( QStringLiteral("dbDriver") ) );

    widget.m_connectionType->setCurrentIndex( AmarokConfigWidget::External );
    QVERIFY( !widget.config().contains( QStringLiteral("dbDriver") ) );
}

void
TestAmarokImporter::configWidgetShouldShowExternalAsDefault()
{
    QVariantMap cfg;
    AmarokConfigWidget widget( cfg );
    QCOMPARE( widget.m_connectionType->currentIndex(),
              static_cast<int>( AmarokConfigWidget::External ) );
}

void
TestAmarokImporter::configWidgetShouldNotBreakOnNonsenseInitialValues()
{
    m_cfg.insert( QStringLiteral("dbName"), QColor( Qt::red ) );
    m_cfg.insert( QStringLiteral("dbPort"), QStringLiteral("nonsensePort") );
    m_cfg.insert( QStringLiteral("dbPath"), reinterpret_cast<qptrdiff>( this ) );

    AmarokConfigWidget widget( m_cfg );

    QVERIFY( !widget.m_databaseName->text().isEmpty() );
    QVERIFY( !widget.m_databaseLocation->text().isEmpty() );
}

void
TestAmarokImporter::configWidgetShouldReadSavedConfig()
{
    m_cfg.insert( QStringLiteral("embedded"), true );
    m_cfg.insert( QStringLiteral("dbName"), QStringLiteral("MyName") );
    m_cfg.insert( QStringLiteral("dbPort"), 19 );
    m_cfg.insert( QStringLiteral("name"), QStringLiteral("theName") );
    AmarokConfigWidget widget( m_cfg );

    QCOMPARE( widget.m_connectionType->currentIndex(),
              static_cast<int>( AmarokConfigWidget::Embedded ) );

    QCOMPARE( widget.m_databaseName->text(), QStringLiteral( "MyName" ) );
    QCOMPARE( widget.m_port->value(), 19 );
    QCOMPARE( widget.m_targetName->text(), QStringLiteral( "theName" ) );

    m_cfg.insert( QStringLiteral("embedded"), false );
    AmarokConfigWidget widgetExternal( m_cfg );

    QCOMPARE( widgetExternal.m_connectionType->currentIndex(),
              static_cast<int>( AmarokConfigWidget::External ) );
}

void
TestAmarokImporter::providerShouldIgnoreConfigsDbDriver()
{
    if( !QFileInfo( QStringLiteral("/usr/sbin/mysqld") ).isExecutable() )
        QSKIP( "/usr/sbin/mysqld is not executable", SkipAll );

    m_cfg.insert( QStringLiteral("dbDriver"), QStringLiteral("QPSQL") );
    m_cfg.insert( QStringLiteral("dbPath"), QString( QCoreApplication::applicationDirPath() +
                            QStringLiteral("/../tests/importers_files/amarok2_mysqle") ) );

    dbProvider = nullptr; // free so we're not stuck waiting for the db file to get freed
    AmarokProvider provider( m_cfg, nullptr );

    // The database isn't accessible by QPSQL driver, but it still should work
    QVERIFY( !provider.artists().empty() );
}

void
TestAmarokImporter::providerShouldHandleNonexistentDbDir()
{
    m_cfg.insert( QStringLiteral("dbPath"), QStringLiteral("/Im/sure/this/wont/exist") );

    AmarokProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestAmarokImporter::providerShouldHandleInvalidDbDir()
{
    m_cfg.insert( QStringLiteral("dbPath"), QApplication::applicationDirPath() );

    AmarokProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestAmarokImporter::providerShouldHandleExternalConnectionError()
{
    m_cfg.insert( QStringLiteral("dbHost"), QStringLiteral("I hope this isn't a valid hostname") );

    AmarokProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestAmarokImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( QStringLiteral("dbDriver"), 19 );
    m_cfg.insert( QStringLiteral("dbName"), QColor( Qt::red ) );
    m_cfg.insert( QStringLiteral("dbPort"), QStringLiteral("nonsensePort") );

    AmarokProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

