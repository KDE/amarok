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

#include "TestITunesImporter.h"

#include "MetaValues.h"
#include "importers/itunes/ITunesConfigWidget.h"
#include "importers/itunes/ITunesProvider.h"

#include <KLocalizedString>

#include <QTest>


QTEST_MAIN( TestITunesImporter )

using namespace StatSyncing;

ProviderPtr
TestITunesImporter::getProvider()
{
    QVariantMap cfg = ITunesConfigWidget( QVariantMap() ).config();
    cfg.insert( "dbPath", QApplication::applicationDirPath()
                          + "/importers_files/iTunes_Music_Library.xml" );

    return ProviderPtr( new ITunesProvider( cfg, nullptr ) );
}

ProviderPtr
TestITunesImporter::getWritableProvider()
{
    QDir base( QCoreApplication::applicationDirPath() );
    base.mkpath( "importers_tmp" );

    const QString dst = base.filePath( "importers_tmp/iTunes_Music_Library.xml" );
    QFile( dst ).remove();
    QFile( base.filePath( "importers_files/iTunes_Music_Library.xml" ) ).copy( dst );

    QVariantMap cfg = ITunesConfigWidget( QVariantMap() ).config();
    cfg.insert( "dbPath", dst);

    return ProviderPtr( new ITunesProvider( cfg, nullptr ) );
}

qint64
TestITunesImporter::reliableStatistics() const
{
    return Meta::valLastPlayed | Meta::valRating | Meta::valPlaycount;
}

bool
TestITunesImporter::hasOddRatings() const
{
    return false; // iTunes actually *has* odd ratings, as it's rating value is in range
                  // 0-100, but it represents ratings as multipliers of 20.
}

void
TestITunesImporter::init()
{
    KLocalizedString::setApplicationDomain("amarok-test");
    m_cfg = ITunesConfigWidget( QVariantMap() ).config();
}

void
TestITunesImporter::providerShouldHandleNonexistentDbFile()
{
    m_cfg.insert( "dbPath", "/wdawd\\wdadwgd/das4hutyf" );

    ITunesProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestITunesImporter::providerShouldHandleInvalidDbFile()
{
    m_cfg.insert( "dbPath", QApplication::applicationFilePath() );

    ITunesProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestITunesImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( "dbPath", "\\wd%aw@d/sdsd2'vodk0-=$$" );
    m_cfg.insert( "name", QColor( Qt::white ) );

    ITunesProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestITunesImporter::providerShouldHandleIllFormedDbFile()
{
    m_cfg.insert( "dbPath", QApplication::applicationDirPath()
                  + "/importers_files/illFormedLibrary.xml" );

    ITunesProvider provider( m_cfg, nullptr );
    QVERIFY( provider.artistTracks( "NonSuch" ).empty() );
}
