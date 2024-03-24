/****************************************************************************************
 * Copyright (c) 2012 Jasneet Singh Bhatti <jazneetbhatti@gmail.com>                    *
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

#ifndef TESTMETACONSTANTS_H
#define TESTMETACONSTANTS_H

#include <QtTest>

class TestMetaConstants : public QObject
{
    Q_OBJECT
    public:
        TestMetaConstants();

    private Q_SLOTS:
        // Data driven tests
        void testNameForField_data();
        void testNameForField();
        void testFieldForName_data();
        void testFieldForName();
        void testI18nForField_data();
        void testI18nForField();
        void testShortI18nForField_data();
        void testShortI18nForField();
        void testPlaylistNameForField_data();
        void testPlaylistNameForField();
        void testFieldForPlaylistName_data();
        void testFieldForPlaylistName();
        void testIconForField_data();
        void testIconForField();

        // Uses Mock classes for testing
        void testValueForField();

    private:
        // Test data shared by testNameForField and testFieldForName
        void dataNameField();

        // Test data shared by testPlaylistNameForField and testFieldForPlaylistName
        void dataPlaylistNameField();
};

#endif // TESTMETACONSTANTS_H
