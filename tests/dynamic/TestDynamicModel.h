/****************************************************************************************
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#ifndef TESTDYNAMICMODEL_H
#define TESTDYNAMICMODEL_H

#include <QtTest/QTest>

class TestDynamicModel : public QObject
{
    Q_OBJECT
public:
    TestDynamicModel();

private slots:
    void init();
    void cleanup();

    void testData();
    void testPlaylistIndex();
    void testSaveActive();
    void testRemoveActive();

private:
    /*
    Collections::SqlCollection *m_collection;
    SqlMountPointManagerMock *m_mpm;
    SqlStorage *m_storage;
    */
};

#endif
