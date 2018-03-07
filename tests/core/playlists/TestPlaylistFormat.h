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

#ifndef TESTPLAYLISTFORMAT_H
#define TESTPLAYLISTFORMAT_H

#include <QtTest>

class TestPlaylistFormat : public QObject
{
    Q_OBJECT

    public:
        TestPlaylistFormat();

    private Q_SLOTS:
        void testGetFormat_data();
        /**
         * Check if the correct extension format for the playlist file is returned
         */
        void testGetFormat();

        void testIsPlaylist_data();
        /**
         * Check if the extension format of the playlist file is supported or not
         */
        void testIsPlaylist();
};

#endif // TESTPLAYLISTFORMAT_H
