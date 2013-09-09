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

#include "TestBansheeImporter.h"

#include "MetaValues.h"
#include "importers/banshee/BansheeConfigWidget.h"
#include "importers/banshee/BansheeProvider.h"

#include <qtest_kde.h>

QTEST_KDEMAIN( TestBansheeImporter, GUI )

using namespace StatSyncing;

ProviderPtr
TestBansheeImporter::getProvider()
{
    QVariantMap cfg = BansheeConfigWidget( QVariantMap() ).config();
    cfg.insert( "dbPath", QApplication::applicationDirPath()
                          + "/importers_files/banshee.db" );

    return ProviderPtr( new BansheeProvider( cfg, 0 ) );
}

qint64
TestBansheeImporter::reliableStatistics() const
{
    return Meta::valLastPlayed | Meta::valPlaycount | Meta::valRating;
}

bool
TestBansheeImporter::hasOddRatings() const
{
    return false;
}

void
TestBansheeImporter::init()
{
    m_cfg = BansheeConfigWidget( QVariantMap() ).config();
}

void
TestBansheeImporter::providerShouldHandleNonexistentDbFile()
{
    m_cfg.insert( "dbPath", "/wdawd\\wdadwgd/das4hutyf" );

    BansheeProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestBansheeImporter::providerShouldHandleInvalidDbFile()
{
    m_cfg.insert( "dbPath", QApplication::applicationFilePath() );

    BansheeProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestBansheeImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( "dbPath", "\\wd%aw@d/sdsd2'vodk0-=$$" );
    m_cfg.insert( "name", QColor( Qt::white ) );

    BansheeProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestBansheeImporter::artistTracksShouldNotReturnTracksNotFromPrimarySource()
{
    ProviderPtr provider( getProvider() );

    const QString artist = "wrongSource";
    QVERIFY( provider->artists().contains( artist ) );
    QVERIFY( provider->artistTracks( artist ).isEmpty() );
}
