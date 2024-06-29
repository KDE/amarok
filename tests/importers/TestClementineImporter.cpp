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

#include "TestClementineImporter.h"

#include "MetaValues.h"
#include "importers/clementine/ClementineConfigWidget.h"
#include "importers/clementine/ClementineProvider.h"

#include <KLocalizedString>

#include <QTest>


QTEST_MAIN( TestClementineImporter )

using namespace StatSyncing;

ProviderPtr
TestClementineImporter::getProvider()
{
    QVariantMap cfg = ClementineConfigWidget( QVariantMap() ).config();
    cfg.insert( QStringLiteral("dbPath"), QString( QApplication::applicationDirPath()
                          + QStringLiteral("/../tests/importers_files/clementine.db") ) );

    return ProviderPtr( new ClementineProvider( cfg, nullptr ) );
}

ProviderPtr
TestClementineImporter::getWritableProvider()
{
    QDir base( QCoreApplication::applicationDirPath() );
    base.mkpath( QStringLiteral("importers_tmp") );

    const QString dst = base.filePath( QStringLiteral("importers_tmp/clementine.db") );
    QFile( dst ).remove();
    QFile( base.filePath( QStringLiteral("../tests/importers_files/clementine.db") ) ).copy( dst );

    QVariantMap cfg = ClementineConfigWidget( QVariantMap() ).config();
    cfg.insert( QStringLiteral("dbPath"), dst );

    return ProviderPtr( new ClementineProvider( cfg, nullptr ) );
}

qint64
TestClementineImporter::reliableStatistics() const
{
    return Meta::valLastPlayed | Meta::valRating | Meta::valPlaycount;
}

void
TestClementineImporter::init()
{
    KLocalizedString::setApplicationDomain("amarok-test");
    m_cfg = ClementineConfigWidget( QVariantMap() ).config();
}

void
TestClementineImporter::providerShouldHandleNonexistentDbFile()
{
    m_cfg.insert( QStringLiteral("dbPath"), QStringLiteral("/wdawd\\wdadwgd/das4hutyf") );

    ClementineProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestClementineImporter::providerShouldHandleInvalidDbFile()
{
    m_cfg.insert( QStringLiteral("dbPath"), QApplication::applicationFilePath() );

    ClementineProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestClementineImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( QStringLiteral("dbPath"), QStringLiteral("\\wd%aw@d/sdsd2'vodk0-=$$") );
    m_cfg.insert( QStringLiteral("name"), QColor( Qt::white ) );

    ClementineProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}
