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

#ifndef TESTMETAFILETRACK_H
#define TESTMETAFILETRACK_H

#include <QtCore/QStringList>

namespace MetaFile {
    class Track;
}

class TestMetaFileTrack : public QObject
{
Q_OBJECT

public:
    TestMetaFileTrack( QStringList testArgumentList, bool stdout );

private slots:
    void initTestCase();
    //methods inherited from Meta::MetaBase
    void testNameAndSetTitle();
    void testPrettyName();
    void testFullPrettyName();
    void testSortableName();

    //methods inherited from Meta::Track
    void testPlayableUrl();
    void testPrettyUrl();
    void testUidUrl();

    void testIsPlayable();
    void testIsEditable();

    void testSetGetAlbum();
    void testSetGetArtist();
    void testSetGetGenre();
    void testSetGetComposer();
    void testSetGetYear();
    void testSetGetComment();
    void testSetGetScore();
    void testSetGetRating();
    void testSetGetTrackNumber();
    void testSetGetDiscNumber();
    void testLength();
    void testFilesize();
    void testSampleRate();
    void testBitrate();
    void testSetGetLastPlayed();
    void testSetGetFirstPlayed();
    void testSetGetPlayCount();
    void testReplayGain();
    void testReplayPeakGain();
    void testType();
    void testCreateDate();

private:
    MetaFile::Track *track;
};

#endif // TESTMETAFILETRACK_H
