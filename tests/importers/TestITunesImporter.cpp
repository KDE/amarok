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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "TestITunesImporter.h"

#include "CollectionTestImpl.h"
#include "MockITunesImporter.h"

#include <qtest_kde.h>

QTEST_KDEMAIN( TestITunesImporter, GUI )

DatabaseImporter*
TestITunesImporter::newInstance()
{
    return new MockITunesImporter( m_localCollection.data() );
}

QString
TestITunesImporter::pathForMetadata( const QString &artist, const QString &album, const QString &title )
{
    return "file:///C:/" + artist + "/" + album + "/" + title + ".mp3";
}

