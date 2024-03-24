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

#ifndef TESTMETAALBUMKEY_H
#define TESTMETAALBUMKEY_H

#include "core/meta/forward_declarations.h"

#include <QtTest>

class TestMetaAlbumKey : public QObject
{
    Q_OBJECT

    public:
        TestMetaAlbumKey();
        ~TestMetaAlbumKey() override;

    private Q_SLOTS:
        void initTestCase();

        /**
         * Test constructor AlbumKey( const AlbumPtr &album )
         */
        void testAlbumKey();
        void testOperatorAssignment();
        void testOperatorLessThan();

    private:
        /**
         * For portability in fetching tracks from disk
         */
        QString dataPath( const QString &relPath );

        Meta::TrackPtr m_track1, m_track2, m_track3;
        Meta::AlbumPtr m_album1, m_album2, m_album3;
};

#endif // TESTMETAALBUMKEY_H
