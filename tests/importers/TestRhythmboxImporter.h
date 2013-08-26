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

#ifndef TEST_RHYTHMBOX_IMPORTER
#define TEST_RHYTHMBOX_IMPORTER

#include <QObject>
#include <QVariantMap>

class TestRhythmboxImporter : public QObject
{
    Q_OBJECT

private:
    QVariantMap m_cfg;

private slots:
    void init();

    void providerShouldHandleInexistantDbFile();
    void providerShouldHandleInvalidDbFile();
    void providerShouldHandleIllFormedDbFile();
    void providerShouldHandleErroneousConfigValues();
    void providerShouldHandleInexistantArtist();
    void artistsShouldReturnExistingArtists();
    void artistTracksShouldReturnPopulatedTracks_data();
    void artistTracksShouldReturnPopulatedTracks();

    void artistTracksShoulsHandleNonexistentStatistics_data();
    void artistTracksShoulsHandleNonexistentStatistics();
};

#endif // TEST_RHYTHMBOX_IMPORTER
