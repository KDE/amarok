/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@asbest-online.de>               *
 *   Copyright (c) 2013 Matěj Laitl <matej@laitl.cz>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef TESTTRACKLOADER_H
#define TESTTRACKLOADER_H

#include <QObject>

class TestTrackLoader : public QObject
{
    Q_OBJECT

    private Q_SLOTS:
        void initTestCase();
        void cleanupTestCase();

        // this in intentionally on top so that it is executed first
        void testFullMetadataInit();
        void testInit();
        void testInitWithPlaylists();
        void testDirectlyPassingPlaylists();

    private:
        QString dataPath( const QString &relPath );
};

#endif // TESTTRACKLOADER_H
