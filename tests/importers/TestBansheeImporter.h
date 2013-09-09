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

#ifndef TEST_BANSHEE_IMPORTER
#define TEST_BANSHEE_IMPORTER

#include "TestImporterBase.h"

#include <QVariantMap>

class TestBansheeImporter : public TestImporterBase
{
    Q_OBJECT

protected:
    StatSyncing::ProviderPtr getProvider();
    qint64 reliableStatistics() const;
    bool hasOddRatings() const;

private:
    QVariantMap m_cfg;

private slots:
    void init();

    void providerShouldHandleNonexistentDbFile();
    void providerShouldHandleInvalidDbFile();
    void providerShouldHandleErroneousConfigValues();

    void artistTracksShouldNotReturnTracksNotFromPrimarySource();
};

#endif // TEST_BANSHEE_IMPORTER
