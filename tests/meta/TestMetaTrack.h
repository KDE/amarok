/***************************************************************************
 *   Copyright (c) 2009 Sven Krohlas <sven@getamarok.com>                  *
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

#ifndef TESTMETATRACK_H
#define TESTMETATRACK_H

#include "core/meta/Meta.h"

#include <QtCore/QObject>
#include <QtCore/QString>

class TestMetaTrack : public QObject
{
Q_OBJECT

public:
    TestMetaTrack();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testPrettyName();
    void testPlayableUrl();
    void testPrettyUrl();
    void testUidUrl();

    void testIsPlayable();
    void testAlbum();
    void testArtist();
    void testComposer();
    void testGenre();
    void testYear();

    void testComment();
    void testSetAndGetScore();
    void testSetAndGetRating();
    void testLength();
    void testFilesize();
    void testSampleRate();
    void testBitrate();
    void testTrackNumber();
    void testDiscNumber();
    void testLastPlayed();
    void testFirstPlayed();
    void testPlayCount();
    void testReplayGain();
    void testReplayPeakGain();
    void testType();

    void testInCollection();
    void testCollection();
    void testSetAndGetCachedLyrics();
    void testOperatorEquals();
    void testLessThan();

private:
    Meta::TrackPtr m_testTrack1;
    Meta::TrackPtr m_testTrack2;

    QString dataPath( const QString &relPath );
};

#endif // TESTMETATRACK_H
