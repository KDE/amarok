/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>       *
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

#include "TestSqlArtist.h"

#include "core/support/Debug.h"
#include "core/meta/Meta.h"
#include "core-impl/meta/file/TagLibUtils.h"
#include "SqlMeta.h"

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestSqlArtist )

//defined in TagLibUtils.h

namespace TagLib
{
    struct FileRef
    {
        //dummy
    };
}

void
Meta::Field::writeFields(const QString &filename, const QVariantMap &changes )
{
    return;
}

void
Meta::Field::writeFields(TagLib::FileRef fileref, const QVariantMap &changes)
{
    return;
}

TestSqlArtist::TestSqlArtist()
{
}

void
TestSqlArtist::testSortableName()
{
    Meta::ArtistPtr artistWithThe( new Meta::SqlArtist( 0, 1, "The Foo" ) );
    QCOMPARE( artistWithThe->sortableName(), QString( "Foo, The" ) );

    Meta::ArtistPtr artistWithoutThe( new Meta::SqlArtist( 0, 1, "No The Foo" ) );
    QCOMPARE( artistWithoutThe->sortableName(), QString( "No The Foo" ) );
}


#include "TestSqlArtist.moc"
