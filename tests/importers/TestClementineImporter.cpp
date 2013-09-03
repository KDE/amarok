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

#include <qtest_kde.h>

QTEST_KDEMAIN( TestClementineImporter, GUI )

using namespace StatSyncing;

ProviderPtr
TestClementineImporter::getProvider()
{
    QVariantMap cfg = ClementineConfigWidget( QVariantMap() ).config();
    cfg.insert( "dbPath", QApplication::applicationDirPath()
                          + "/importers_files/clementine.db" );

    return ProviderPtr( new ClementineProvider( cfg, 0 ) );
}

qint64
TestClementineImporter::reliableStatistics() const
{
    return Meta::valLastPlayed | Meta::valRating | Meta::valPlaycount;
}

void
TestClementineImporter::init()
{
    m_cfg = ClementineConfigWidget( QVariantMap() ).config();
}

void
TestClementineImporter::providerShouldHandleNonexistentDbFile()
{
    m_cfg.insert( "dbPath", "/wdawd\\wdadwgd/das4hutyf" );

    ClementineProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestClementineImporter::providerShouldHandleInvalidDbFile()
{
    m_cfg.insert( "dbPath", QApplication::applicationFilePath() );

    ClementineProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}

void
TestClementineImporter::providerShouldHandleErroneousConfigValues()
{
    m_cfg.insert( "dbPath", "\\wd%aw@d/sdsd2'vodk0-=$$" );
    m_cfg.insert( "name", QColor( Qt::white ) );

    ClementineProvider provider( m_cfg, 0 );
    QVERIFY( provider.artists().isEmpty() );
}
