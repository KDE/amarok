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

#ifndef TESTPLSPLAYLIST_H
#define TESTPLSPLAYLIST_H

#include "core/playlists/impl/file/pls/PLSPlaylist.h"
#include "TestBase.h"

#include <QtCore/QStringList>

class TestPLSPlaylist : public TestBase
{
Q_OBJECT

public:
    TestPLSPlaylist( const QStringList args, const QString &logPath );

private slots:
    void initTestCase();

    void setAndGetName();
    void prettyName();
    void tracks();
    void retrievableUrl();
    void isWritable();
    void save();

private:
    Meta::PLSPlaylist m_testPlaylist1;
};

#endif // TESTPLSPLAYLIST_H
