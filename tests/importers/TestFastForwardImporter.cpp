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

#include "TestFastForwardImporter.h"

#include "CollectionTestImpl.h"
#include "core-impl/collections/support/CollectionManager.h"
#include "MockFastForwardImporter.h"
#include "mocks/MetaMock.h"

#include <qtest_kde.h>

QTEST_KDEMAIN( TestFastForwardImporter, GUI )

using namespace Collections;

DatabaseImporter*
TestFastForwardImporter::newInstance()
{
    return new MockFastForwardImporter;
}

QString
TestFastForwardImporter::pathForMetadata( const QString &artist, const QString &album,
                                          const QString &title )
{
    return "/" + artist + "/" + album + "/" + title + ".mp3";
}

void
TestFastForwardImporter::importerShouldMatchByMetadata()
{
    // TODO: Currently matching is very strict, this will change with rewrite on StatSyncing
    Meta::TrackPtr track = m_trackForName["The Unforgiven"];
    MetaMock *mock = dynamic_cast<MetaMock*>( track.data() );

    mock->m_data.remove( Meta::Field::URL );
    mock->m_data.insert( Meta::Field::TRACKNUMBER, 4 );
    mock->m_data.insert( Meta::Field::DISCNUMBER, 0 );
    mock->m_data.insert( Meta::Field::FILESIZE, 0 );
    mock->m_composer = Meta::ComposerPtr(
                new MockComposer( "James Hetfield, Kirk Hammett, Lars Ulrich" ) );
    mock->m_genre = Meta::GenrePtr( new MockGenre( "Metal" ) );

    blockingImport();

    QCOMPARE( track->statistics()->rating(), 5 );
}

void
TestFastForwardImporter::importerSpecificStatsShouldBeImported()
{
    blockingImport();
    QFETCH( QString, title );
    Meta::StatisticsPtr stat = m_trackForName[title]->statistics();
    QTEST( stat->firstPlayed(), "firstPlayed" );
    QTEST( stat->score(), "score" );
}
