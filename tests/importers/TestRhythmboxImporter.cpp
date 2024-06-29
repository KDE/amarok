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

#include "TestRhythmboxImporter.h"

#include "MetaValues.h"
#include "importers/rhythmbox/RhythmboxConfigWidget.h"
#include "importers/rhythmbox/RhythmboxProvider.h"

#include <KLocalizedString>

#include <QTest>

QTEST_MAIN( TestRhythmboxImporter )

using namespace StatSyncing;

ProviderPtr
TestRhythmboxImporter::getProvider()
{
    QVariantMap cfg = RhythmboxConfigWidget( QVariantMap() ).config();
    cfg.insert( QStringLiteral("dbPath"), QString( QApplication::applicationDirPath()
                          + QStringLiteral("/../tests/importers_files/rhythmdb.xml") ) );

    return ProviderPtr( new RhythmboxProvider( cfg, nullptr ) );
}

ProviderPtr
TestRhythmboxImporter::getWritableProvider()
{
    QDir base( QCoreApplication::applicationDirPath() );
    base.mkpath( QStringLiteral("importers_tmp") );

    const QString dst = base.filePath( QStringLiteral("importers_tmp/rhythmdb.xml") );
    QFile( dst ).remove();
    QFile( base.filePath( QStringLiteral("../tests/importers_files/rhythmdb.xml") ) ).copy( dst );

    QVariantMap cfg = RhythmboxConfigWidget( QVariantMap() ).config();
    cfg.insert( QStringLiteral("dbPath"), dst );

    return ProviderPtr( new RhythmboxProvider( cfg, nullptr ) );
}

qint64
TestRhythmboxImporter::reliableStatistics() const
{
    return Meta::valLastPlayed | Meta::valRating | Meta::valPlaycount;
}

bool
TestRhythmboxImporter::hasOddRatings() const
{
    return false;
}

void
TestRhythmboxImporter::init()
{
    KLocalizedString::setApplicationDomain("amarok-test");
    m_cfg = RhythmboxConfigWidget( QVariantMap() ).config();
}

void
TestRhythmboxImporter::providerShouldHandleNonexistentDbFile()
{
    m_cfg.insert( QStringLiteral("dbPath"), QStringLiteral("/wdawd\\wdadwgd/das4hutyf") );

    RhythmboxProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestRhythmboxImporter::providerShouldHandleInvalidDbFile()
{
    m_cfg.insert( QStringLiteral("dbPath"), QApplication::applicationFilePath() );

    RhythmboxProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestRhythmboxImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( QStringLiteral("dbPath"), QStringLiteral("\\wd%aw@d/sdsd2'vodk0-=$$") );
    m_cfg.insert( QStringLiteral("name"), QColor( Qt::white ) );

    RhythmboxProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestRhythmboxImporter::providerShouldHandleIllFormedDbFile()
{
    m_cfg.insert( QStringLiteral("dbPath"), QString( QApplication::applicationDirPath()
                  + QStringLiteral("/importers_files/illFormedLibrary.xml") ) );

    RhythmboxProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artistTracks( QStringLiteral("NonSuch") ).empty() );
}
