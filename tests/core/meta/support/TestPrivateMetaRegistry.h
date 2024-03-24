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

#ifndef TESTPRIVATEMETAREGISTRY_H
#define TESTPRIVATEMETAREGISTRY_H

#include "core/meta/forward_declarations.h"

#include <QtTest>

using namespace Meta;

class TestPrivateMetaRegistry : public QObject
{
    Q_OBJECT

    public:
        TestPrivateMetaRegistry();
        ~TestPrivateMetaRegistry() override;

    private Q_SLOTS:
        void initTestCase();

        /**
         * Data driven testing
         */
        void testInsertAlbum_data();
        void testInsertAlbum();
        void testInsertArtist_data();
        void testInsertArtist();
        void testInsertComposer_data();
        void testInsertComposer();
        void testInsertGenre_data();
        void testInsertGenre();
        void testInsertYear_data();
        void testInsertYear();

        /**
         * Test if the getters correctly return a pointer to null for a non-existing
         * combination of owner and key.
         * This is done in a separate method to avoid repetition of this verification
         * for each instance of the test data
         */
        void testNull();

    private:
        /**
         * For portability in fetching tracks from disk
         */
        QString datapath( const QString &relPath );

        /**
         * All _data() methods use this to create test data
         */
        void createTestData();

        /**
         * Test tracks required for generating test data
         */
        TrackPtr m_track1, m_track2, m_track3, m_track4, m_track5;
};

#endif // TESTPRIVATEMETAREGISTRY_H
