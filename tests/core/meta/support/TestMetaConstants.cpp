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

#include "TestMetaConstants.h"

#include "core/meta/support/MetaConstants.h"
#include "mocks/MetaMock.h"
#include "MetaValues.h"
#include "FileType.h"

#include <QDateTime>

#include <KLocalizedString>


using namespace Meta;

QTEST_MAIN( TestMetaConstants )

/* Just for clarification. This is not how you would normally write an auto test.
   You don't write a switch just to test if the switch returns the correct values.

   You also don't test localized texts, since you would have to run the test
   in all available languages. There are other methods to test if you missed to
   translate something. This is not it.

   In my case the test failed three times. Always a false positive.
*/

TestMetaConstants::TestMetaConstants()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

void
TestMetaConstants::dataNameField()
{
    QTest::addColumn<qint64>( "field" );
    QTest::addColumn<QString>( "name" );

    QTest::newRow( "anything" )      << qint64( 0 )            << "anything";
    QTest::newRow( "filename" )      << Meta::valUrl           << "filename";
    QTest::newRow( "title" )         << Meta::valTitle         << "title";
    QTest::newRow( "artist" )        << Meta::valArtist        << "artist";
    QTest::newRow( "album" )         << Meta::valAlbum         << "album";
    QTest::newRow( "genre" )         << Meta::valGenre         << "genre";
    QTest::newRow( "composer" )      << Meta::valComposer      << "composer";
    QTest::newRow( "year" )          << Meta::valYear          << "year";
    QTest::newRow( "comment" )       << Meta::valComment       << "comment";
    QTest::newRow( "discnumber" )    << Meta::valDiscNr        << "discnumber";
    QTest::newRow( "bpm" )           << Meta::valBpm           << "bpm";
    QTest::newRow( "length" )        << Meta::valLength        << "length";
    QTest::newRow( "bitrate" )       << Meta::valBitrate       << "bitrate";
    QTest::newRow( "samplerate" )    << Meta::valSamplerate    << "samplerate";
    QTest::newRow( "filesize" )      << Meta::valFilesize      <<  "filesize";
    QTest::newRow( "format" )        << Meta::valFormat        <<  "format";
    QTest::newRow( "added" )         << Meta::valCreateDate    <<  "added";
    QTest::newRow( "score" )         << Meta::valScore         <<  "score";
    QTest::newRow( "rating" )        << Meta::valRating        <<  "rating";
    QTest::newRow( "firstplay" )     << Meta::valFirstPlayed   <<  "firstplay";
    QTest::newRow( "lastplay" )      << Meta::valLastPlayed    <<  "lastplay";
    QTest::newRow( "playcount" )     << Meta::valPlaycount     <<  "playcount";
    QTest::newRow( "uniqueid" )      << Meta::valUniqueId      <<  "uniqueid";

    QTest::newRow( "trackgain" )     << Meta::valTrackGain     <<  "trackgain";
    QTest::newRow( "trackgainpeak" ) << Meta::valTrackGainPeak <<  "trackgainpeak";
    QTest::newRow( "albumgain" )     << Meta::valAlbumGain     <<  "albumgain";
    QTest::newRow( "albumgainpeak" ) << Meta::valAlbumGainPeak <<  "albumgainpeak";

    QTest::newRow( "albumartist" )   << Meta::valAlbumArtist   <<  "albumartist";
    QTest::newRow( "label" )         << Meta::valLabel         <<  "label";
    QTest::newRow( "modified" )      << Meta::valModified      <<  "modified";
}

void
TestMetaConstants::testNameForField_data()
{
    dataNameField();

    // Some test cases that can't be shared
    QTest::newRow( "tracknr" )       << Meta::valTrackNr       << "tracknr";
    QTest::newRow( "default" )       << qint64( -1 )           <<  "";
}

void
TestMetaConstants::testNameForField()
{
    QFETCH( qint64, field );
    QFETCH( QString, name );

    QCOMPARE( nameForField( field ), name );
}

void
TestMetaConstants::testFieldForName_data()
{
    dataNameField();

    // Some test cases that can't be shared
    QTest::newRow( "codec" )                                       << Meta::valFormat        <<  "codec";
    QTest::newRow( "first" )                                       << Meta::valFirstPlayed   <<  "first";
    QTest::newRow( "played" )                                      << Meta::valLastPlayed    <<  "played";
    QTest::newRow( "tracknumber" )                                 << Meta::valTrackNr       << "tracknumber";

    // Mixed case
    QTest::newRow( "Filename" )                                    << Meta::valUrl           << "Filename";
    QTest::newRow( "TrackGain" )                                   << Meta::valTrackGain     << "TrackGain";

    // No match
    QTest::newRow( "noMatch" )                                     << qint64( 0 )            << "noMatch";

    QTest::newRow( "shortI18nForField( 0 )" )                      << qint64( 0 )            << shortI18nForField( 0 );
    QTest::newRow( "shortI18nForField( Meta::valUrl )" )           << Meta::valUrl           << shortI18nForField( Meta::valUrl );
    QTest::newRow( "shortI18nForField( Meta::valTitle )" )         << Meta::valTitle         << shortI18nForField( Meta::valTitle );
    QTest::newRow( "shortI18nForField( Meta::valArtist )" )        << Meta::valArtist        << shortI18nForField( Meta::valArtist );
    QTest::newRow( "shortI18nForField( Meta::valAlbum )" )         << Meta::valAlbum         << shortI18nForField( Meta::valAlbum );
    QTest::newRow( "shortI18nForField( Meta::valGenre )" )         << Meta::valGenre         << shortI18nForField( Meta::valGenre );
    QTest::newRow( "shortI18nForField( Meta::valComposer )" )      << Meta::valComposer      << shortI18nForField( Meta::valComposer );
    QTest::newRow( "shortI18nForField( Meta::valYear )" )          << Meta::valYear          << shortI18nForField( Meta::valYear );
    QTest::newRow( "shortI18nForField( Meta::valComment )" )       << Meta::valComment       << shortI18nForField( Meta::valComment );
    QTest::newRow( "shortI18nForField( Meta::valDiscNr )" )        << Meta::valDiscNr        << shortI18nForField( Meta::valDiscNr );
    QTest::newRow( "shortI18nForField( Meta::valBpm )" )           << Meta::valBpm           << shortI18nForField( Meta::valBpm );
    QTest::newRow( "shortI18nForField( Meta::valLength )" )        << Meta::valLength        << shortI18nForField( Meta::valLength );
    QTest::newRow( "shortI18nForField( Meta::valBitrate )" )       << Meta::valBitrate       << shortI18nForField( Meta::valBitrate );
    QTest::newRow( "shortI18nForField( Meta::valSamplerate )" )    << Meta::valSamplerate    << shortI18nForField( Meta::valSamplerate );
    QTest::newRow( "shortI18nForField( Meta::valFilesize )" )      << Meta::valFilesize      << shortI18nForField( Meta::valFilesize );
    QTest::newRow( "shortI18nForField( Meta::valFormat )" )        << Meta::valFormat        << shortI18nForField( Meta::valFormat );
    QTest::newRow( "shortI18nForField( Meta::valCreateDate )" )    << Meta::valCreateDate    << shortI18nForField( Meta::valCreateDate );
    QTest::newRow( "shortI18nForField( Meta::valScore )" )         << Meta::valScore         << shortI18nForField( Meta::valScore );
    QTest::newRow( "shortI18nForField( Meta::valRating )" )        << Meta::valRating        << shortI18nForField( Meta::valRating );
    QTest::newRow( "shortI18nForField( Meta::valFirstPlayed )" )   << Meta::valFirstPlayed   << shortI18nForField( Meta::valFirstPlayed );
    QTest::newRow( "shortI18nForField( Meta::valLastPlayed )" )    << Meta::valLastPlayed    << shortI18nForField( Meta::valLastPlayed );
    QTest::newRow( "shortI18nForField( Meta::valPlaycount )" )     << Meta::valPlaycount     << shortI18nForField( Meta::valPlaycount );
    QTest::newRow( "shortI18nForField( Meta::valUniqueId )" )      << Meta::valUniqueId      << shortI18nForField( Meta::valUniqueId );

    QTest::newRow( "shortI18nForField( Meta::valTrackGain )" )     << Meta::valTrackGain     << shortI18nForField( Meta::valTrackGain );
    QTest::newRow( "shortI18nForField( Meta::valTrackGainPeak )" ) << Meta::valTrackGainPeak << shortI18nForField( Meta::valTrackGainPeak );
    QTest::newRow( "shortI18nForField( Meta::valAlbumGain )" )     << Meta::valAlbumGain     << shortI18nForField( Meta::valAlbumGain );
    QTest::newRow( "shortI18nForField( Meta::valAlbumGainPeak )" ) << Meta::valAlbumGainPeak << shortI18nForField( Meta::valAlbumGainPeak );

    QTest::newRow( "shortI18nForField( Meta::valAlbumArtist )" )   << Meta::valAlbumArtist   << shortI18nForField( Meta::valAlbumArtist );
    QTest::newRow( "shortI18nForField( Meta::valLabel )" )         << Meta::valLabel         << shortI18nForField( Meta::valLabel );
    QTest::newRow( "shortI18nForField( Meta::valModified )" )      << Meta::valModified      << shortI18nForField( Meta::valModified );

    QTest::newRow( "shortI18nForField( Meta::valFormat )" )        << Meta::valFormat        << shortI18nForField( Meta::valFormat );
    QTest::newRow( "shortI18nForField( Meta::valFirstPlayed )" )   << Meta::valFirstPlayed   << shortI18nForField( Meta::valFirstPlayed );
    QTest::newRow( "shortI18nForField( Meta::valLastPlayed )" )    << Meta::valLastPlayed    << shortI18nForField( Meta::valLastPlayed );
    QTest::newRow( "shortI18nForField( Meta::valTrackNr )" )       << Meta::valTrackNr       << shortI18nForField( Meta::valTrackNr );
}

void
TestMetaConstants::testFieldForName()
{
    QFETCH( QString, name );
    QFETCH( qint64, field );

    QCOMPARE( fieldForName( name ), field );

    // Uppercase should also work
    QCOMPARE( fieldForName( name.toUpper() ), field );
}

void
TestMetaConstants::testI18nForField_data()
{
    QTest::addColumn<qint64>( "field" );
    QTest::addColumn<QString>( "localized" );

    QTest::newRow( "anything" )      << qint64( 0 )            << i18nc( "The field name in case nothing specific is selected e.g. in the automatic playlist generator", "anything" );
    QTest::newRow( "filename" )      << Meta::valUrl           << i18nc( "The name of the file this track is stored in", "File Name" );
    QTest::newRow( "title" )         << Meta::valTitle         << i18n( "Title" );
    QTest::newRow( "artist" )        << Meta::valArtist        << i18n( "Artist" );
    QTest::newRow( "album" )         << Meta::valAlbum         << i18n( "Album" );
    QTest::newRow( "genre" )         << Meta::valGenre         << i18n( "Genre" );
    QTest::newRow( "composer" )      << Meta::valComposer      << i18n( "Composer" );
    QTest::newRow( "year" )          << Meta::valYear          << i18n( "Year" );
    QTest::newRow( "comment" )       << Meta::valComment       << i18n( "Comment" );
    QTest::newRow( "tracknumber" )   << Meta::valTrackNr       << i18n( "Track Number" );
    QTest::newRow( "discnumber" )    << Meta::valDiscNr        << i18n( "Disc Number" );
    QTest::newRow( "bpm" )           << Meta::valBpm           << i18n( "Bpm" );
    QTest::newRow( "length" )        << Meta::valLength        << i18n( "Length" );
    QTest::newRow( "bitrate" )       << Meta::valBitrate       << i18n( "Bit Rate" );
    QTest::newRow( "samplerate" )    << Meta::valSamplerate    << i18n( "Sample Rate" );
    QTest::newRow( "filesize" )      << Meta::valFilesize      << i18n( "File Size" );
    QTest::newRow( "format" )        << Meta::valFormat        << i18n( "Format" );
    QTest::newRow( "added" )         << Meta::valCreateDate    << i18n( "Added to Collection" );
    QTest::newRow( "score" )         << Meta::valScore         << i18n( "Score" );
    QTest::newRow( "rating" )        << Meta::valRating        << i18n( "Rating" );
    QTest::newRow( "firstplay" )     << Meta::valFirstPlayed   << i18n( "First Played" );
    QTest::newRow( "lastplay" )      << Meta::valLastPlayed    << i18n( "Last Played" );
    QTest::newRow( "playcount" )     << Meta::valPlaycount     << i18n( "Playcount" );
    QTest::newRow( "uniqueid" )      << Meta::valUniqueId      << i18n( "Unique Id" );

    QTest::newRow( "trackgain" )     << Meta::valTrackGain     << i18n( "Track Gain" );
    QTest::newRow( "trackgainpeak" ) << Meta::valTrackGainPeak << i18n( "Track Gain Peak" );
    QTest::newRow( "albumgain" )     << Meta::valAlbumGain     << i18n( "Album Gain" );
    QTest::newRow( "albumgainpeak" ) << Meta::valAlbumGainPeak << i18n( "Album Gain Peak" );

    QTest::newRow( "albumartist" )   << Meta::valAlbumArtist   << i18n( "Album Artist" );
    QTest::newRow( "label" )         << Meta::valLabel         << i18n( "Label" );
    QTest::newRow( "modified" )      << Meta::valModified      << i18n( "Last Modified" );
    QTest::newRow( "default" )       << qint64( -1 )           << QString();
}

void
TestMetaConstants::testI18nForField()
{
    QFETCH( qint64, field );
    QFETCH( QString, localized );

    QCOMPARE( i18nForField( field ), localized );
}

void
TestMetaConstants::testShortI18nForField_data()
{
    QTest::addColumn<qint64>( "field" );
    QTest::addColumn<QString>( "shortname" );

    QTest::newRow( "anything" )      << qint64( 0 )            << i18nc("The field name in case nothing specific is selected e.g. in the automatic playlist generator. Use a one word translation.", "anything");
    QTest::newRow( "filename" )      << Meta::valUrl           << i18nc("One word translation used in the collection filter. The name of the file this track is stored in", "filename" );
    QTest::newRow( "title" )         << Meta::valTitle         << i18nc("One word translation used in the collection filter", "title");
    QTest::newRow( "artist" )        << Meta::valArtist        << i18nc("One word translation used in the collection filter", "artist");
    QTest::newRow( "album" )         << Meta::valAlbum         << i18nc("One word translation used in the collection filter", "album");
    QTest::newRow( "genre" )         << Meta::valGenre         << i18nc("One word translation used in the collection filter", "genre");
    QTest::newRow( "composer" )      << Meta::valComposer      << i18nc("One word translation used in the collection filter", "composer");
    QTest::newRow( "year" )          << Meta::valYear          << i18nc("One word translation used in the collection filter", "year");
    QTest::newRow( "comment" )       << Meta::valComment       << i18nc("One word translation used in the collection filter", "comment");
    QTest::newRow( "tracknumber" )   << Meta::valTrackNr       << i18nc("One word translation used in the collection filter", "tracknumber");
    QTest::newRow( "discnumber" )    << Meta::valDiscNr        << i18nc("One word translation used in the collection filter", "discnumber");
    QTest::newRow( "bpm" )           << Meta::valBpm           << i18nc("One word translation used in the collection filter", "bpm");
    QTest::newRow( "length" )        << Meta::valLength        << i18nc("One word translation used in the collection filter", "length");
    QTest::newRow( "bitrate" )       << Meta::valBitrate       << i18nc("One word translation used in the collection filter", "bitrate");
    QTest::newRow( "samplerate" )    << Meta::valSamplerate    << i18nc("One word translation used in the collection filter", "samplerate");
    QTest::newRow( "filesize" )      << Meta::valFilesize      << i18nc("One word translation used in the collection filter", "filesize");
    QTest::newRow( "format" )        << Meta::valFormat        << i18nc("One word translation used in the collection filter", "format");
    QTest::newRow( "added" )         << Meta::valCreateDate    << i18nc("One word translation used in the collection filter", "added");
    QTest::newRow( "score" )         << Meta::valScore         << i18nc("One word translation used in the collection filter", "score");
    QTest::newRow( "rating" )        << Meta::valRating        << i18nc("One word translation used in the collection filter", "rating");
    QTest::newRow( "firstplay" )     << Meta::valFirstPlayed   << i18nc("One word translation used in the collection filter. First played time / access date", "firstplay");
    QTest::newRow( "lastplay" )      << Meta::valLastPlayed    << i18nc("One word translation used in the collection filter. Last played time / access date", "lastplay");
    QTest::newRow( "playcount" )     << Meta::valPlaycount     << i18nc("One word translation used in the collection filter", "playcount");
    QTest::newRow( "uniqueid" )      << Meta::valUniqueId      << i18nc("One word translation used in the collection filter", "uniqueid");

    QTest::newRow( "trackgain" )     << Meta::valTrackGain     << i18nc("One word translation used in the collection filter", "trackgain");
    QTest::newRow( "trackgainpeak" ) << Meta::valTrackGainPeak << i18nc("One word translation used in the collection filter", "trackgainpeak");
    QTest::newRow( "albumgain" )     << Meta::valAlbumGain     << i18nc("One word translation used in the collection filter", "albumgain");
    QTest::newRow( "albumgainpeak" ) << Meta::valAlbumGainPeak << i18nc("One word translation used in the collection filter", "albumgainpeak");

    QTest::newRow( "albumartist" )   << Meta::valAlbumArtist   << i18nc("One word translation used in the collection filter", "albumartist");
    QTest::newRow( "label" )         << Meta::valLabel         << i18nc("One word translation used in the collection filter", "label");
    QTest::newRow( "modified" )      << Meta::valModified      << i18nc("One word translation used in the collection filter", "modified");
    QTest::newRow( "default" )       << qint64( -1 )           << QString();
}

void
TestMetaConstants::testShortI18nForField()
{
    QFETCH( qint64, field );
    QFETCH( QString, shortname );

    QCOMPARE( shortI18nForField( field ), shortname );
}

void
TestMetaConstants::dataPlaylistNameField()
{
    QTest::addColumn<qint64>( "field" );
    QTest::addColumn<QString>( "playlist" );

    QTest::newRow( "anything" )      << qint64( 0 )            << "anything";
    QTest::newRow( "url" )           << Meta::valUrl           << "url";
    QTest::newRow( "title" )         << Meta::valTitle         << "title";
    QTest::newRow( "artist" )        << Meta::valArtist        << "artist name";
    QTest::newRow( "album" )         << Meta::valAlbum         << "album name";
    QTest::newRow( "genre" )         << Meta::valGenre         << "genre";
    QTest::newRow( "composer" )      << Meta::valComposer      << "composer";
    QTest::newRow( "year" )          << Meta::valYear          << "year";
    QTest::newRow( "comment" )       << Meta::valComment       << "comment";
    QTest::newRow( "tracknumber" )   << Meta::valTrackNr       << "track number";
    QTest::newRow( "discnumber" )    << Meta::valDiscNr        << "disc number";
    QTest::newRow( "bpm" )           << Meta::valBpm           << "bpm";
    QTest::newRow( "length" )        << Meta::valLength        << "length";
    QTest::newRow( "bitrate" )       << Meta::valBitrate       << "bit rate";
    QTest::newRow( "samplerate" )    << Meta::valSamplerate    << "sample rate";
    QTest::newRow( "filesize" )      << Meta::valFilesize      <<  "file size";
    QTest::newRow( "format" )        << Meta::valFormat        <<  "format";
    QTest::newRow( "createdate" )    << Meta::valCreateDate    <<  "create date";
    QTest::newRow( "score" )         << Meta::valScore         <<  "score";
    QTest::newRow( "rating" )        << Meta::valRating        <<  "rating";
    QTest::newRow( "firstplay" )     << Meta::valFirstPlayed   <<  "first played";
    QTest::newRow( "lastplay" )      << Meta::valLastPlayed    <<  "last played";
    QTest::newRow( "playcount" )     << Meta::valPlaycount     <<  "play count";
    QTest::newRow( "uniqueid" )      << Meta::valUniqueId      <<  "unique id";

    QTest::newRow( "trackgain" )     << Meta::valTrackGain     <<  "track gain";
    QTest::newRow( "trackgainpeak" ) << Meta::valTrackGainPeak <<  "track gain peak";
    QTest::newRow( "albumgain" )     << Meta::valAlbumGain     <<  "album gain";
    QTest::newRow( "albumgainpeak" ) << Meta::valAlbumGainPeak <<  "album gain peak";

    QTest::newRow( "albumartist" )   << Meta::valAlbumArtist   <<  "album artist name";
    QTest::newRow( "label" )         << Meta::valLabel         <<  "label";
    QTest::newRow( "modified" )      << Meta::valModified      <<  "modified";
}

void
TestMetaConstants::testPlaylistNameForField_data()
{
    dataPlaylistNameField();

    // Test case that can't be shared
    QTest::newRow( "default" ) << qint64( -1 ) <<  "";
}

void
TestMetaConstants::testPlaylistNameForField()
{
    QFETCH( qint64, field );
    QFETCH( QString, playlist );

    QCOMPARE( playlistNameForField( field ), playlist );
}

void
TestMetaConstants::testFieldForPlaylistName_data()
{
    dataPlaylistNameField();

    // Some test cases that can't be shared
    QTest::newRow( "nomatch" ) << qint64( 0 ) << "nomatch";

    // Playlist name should not be in uppercase or mixed case, returns 0
    QTest::newRow( "NOMATCH" ) << qint64( 0 ) << "NOMATCH";
    QTest::newRow( "noMatch" ) << qint64( 0 ) << "noMatch";
}

void
TestMetaConstants::testFieldForPlaylistName()
{
    QFETCH( QString, playlist );
    QFETCH( qint64, field );

    QCOMPARE( fieldForPlaylistName( playlist ), field );
}

void
TestMetaConstants::testIconForField_data()
{
    QTest::addColumn<qint64>( "field" );
    QTest::addColumn<QString>( "icon" );

    QTest::newRow( "url" )         << Meta::valUrl         << "filename-space-amarok";
    QTest::newRow( "title" )       << Meta::valTitle       << "filename-title-amarok";
    QTest::newRow( "artist" )      << Meta::valArtist      << "filename-artist-amarok";
    QTest::newRow( "albumartist" ) << Meta::valAlbumArtist << "filename-artist-amarok";
    QTest::newRow( "album" )       << Meta::valAlbum       << "filename-album-amarok";
    QTest::newRow( "genre" )       << Meta::valGenre       << "filename-genre-amarok";
    QTest::newRow( "composer" )    << Meta::valComposer    << "filename-composer-amarok";
    QTest::newRow( "year" )        << Meta::valYear        << "filename-year-amarok";
    QTest::newRow( "modified" )    << Meta::valModified    << "filename-year-amarok";
    QTest::newRow( "createdate" )  << Meta::valCreateDate  << "filename-year-amarok";
    QTest::newRow( "comment" )     << Meta::valComment     << "amarok_comment";
    QTest::newRow( "playcount" )   << Meta::valPlaycount   << "amarok_playcount";
    QTest::newRow( "tracknumber" ) << Meta::valTrackNr     << "filename-track-amarok";
    QTest::newRow( "discnumber" )  << Meta::valDiscNr      << "filename-discnumber-amarok";
    QTest::newRow( "bpm" )         << Meta::valBpm         << "filename-bpm-amarok";
    QTest::newRow( "length" )      << Meta::valLength      << "chronometer";
    QTest::newRow( "bitrate" )     << Meta::valBitrate     << "audio-x-generic";
    QTest::newRow( "samplerate" )  << Meta::valSamplerate  << "filename-sample-rate";
    QTest::newRow( "format" )      << Meta::valFormat      << "filename-filetype-amarok";
    QTest::newRow( "score" )       << Meta::valScore       << "emblem-favorite";
    QTest::newRow( "rating" )      << Meta::valRating      << "rating";
    QTest::newRow( "firstplay" )   << Meta::valFirstPlayed << "filename-last-played";
    QTest::newRow( "lastplay" )    << Meta::valLastPlayed  << "filename-last-played";
    QTest::newRow( "label" )       << Meta::valLabel       << "label-amarok";
    QTest::newRow( "filesize" )    << Meta::valFilesize    << "info-amarok";
    QTest::newRow( "default" )     << qint64( -1 )         << "";
}

void
TestMetaConstants::testIconForField()
{
    QFETCH( qint64, field );
    QFETCH( QString, icon );

    QCOMPARE( iconForField( field ), icon );

}

void
TestMetaConstants::testValueForField()
{
    Meta::TrackPtr trackPtr;

    // When track is null, an invalid QVariant is returned
    trackPtr = nullptr;
    QVERIFY( !valueForField( 0, trackPtr ).isValid() );

    // Set up mock track details and create mock track
    QVariantMap trackData;
    trackData[ Meta::Field::URL ] = QUrl(QStringLiteral("file:///test/url"));
    trackData[ Meta::Field::TITLE ] = QStringLiteral("test track");
    trackData[ Meta::Field::COMMENT ] = QStringLiteral("test comment");
    trackData[ Meta::Field::TRACKNUMBER ] = 1;
    trackData[ Meta::Field::DISCNUMBER ] = 1;
    trackData[ Meta::Field::BPM ] = qreal( 1 );
    trackData[ Meta::Field::LENGTH ] = qint64( 1 );
    trackData[ Meta::Field::BITRATE ] = 1;
    trackData[ Meta::Field::SAMPLERATE ] = 1;
    trackData[ Meta::Field::FILESIZE ] = 1;
    trackData[ Meta::Field::SCORE ] = double( 1 );
    trackData[ Meta::Field::RATING ] = 1;
    trackData[ Meta::Field::FIRST_PLAYED ] = QDate( 2012, 1, 1).startOfDay();
    trackData[ Meta::Field::LAST_PLAYED ] = QDate( 2012, 1, 1).startOfDay();
    trackData[ Meta::Field::PLAYCOUNT ] = 1;
    trackData[ Meta::Field::UNIQUEID ] = QStringLiteral("test uid");

    MetaMock *testTrack = new MetaMock( trackData );

    // Associate track with album, artist, etc.
    MockAlbum *testAlbum = new MockAlbum( QStringLiteral("test album") );
    MockArtist *testArtist = new MockArtist( QStringLiteral("test artist") );
    MockComposer *testComposer = new MockComposer( QStringLiteral("test composer") );
    MockGenre *testGenre = new MockGenre( QStringLiteral("test genre") );
    MockYear *testYear = new MockYear( QStringLiteral("2012") );
    MockLabel *testLabel1 = new MockLabel( QStringLiteral("test label 1") );
    MockLabel *testLabel2 = new MockLabel( QStringLiteral("test label 2") );
    MockLabel *testLabel3 = new MockLabel( QStringLiteral("test label 3") );

    // For valAlbumArtist
    testAlbum->m_albumArtist = testArtist;

    testTrack->m_album = testAlbum;
    testTrack->m_artist = testArtist;
    testTrack->m_composer = testComposer;
    testTrack->m_genre = testGenre;
    testTrack->m_year = testYear;
    testTrack->m_labels << Meta::LabelPtr( testLabel1 )
                        << Meta::LabelPtr( testLabel2 )
                        << Meta::LabelPtr( testLabel3 );

    // Make the track pointer point to the created track
    trackPtr = Meta::TrackPtr( testTrack );

    // Case 0
    QVariant trackValue = valueForField( qint64( 0 ), trackPtr );
    QVERIFY( trackValue.toStringList().contains( trackData.value( Meta::Field::URL ).value<QUrl>().path()
        + trackData.value( Meta::Field::TITLE ).toString()
        + trackData.value( Meta::Field::COMMENT ).toString() ) );
    QVERIFY( trackValue.toStringList().contains( testAlbum->name() ) );
    QVERIFY( trackValue.toStringList().contains( testArtist->name() ) );
    QVERIFY( trackValue.toStringList().contains( testGenre->name() ) );

    // Case Meta::valUrl
    trackValue = valueForField( Meta::valUrl, trackPtr );
    QVERIFY( trackValue.toString() == trackData.value( Meta::Field::URL ).value<QUrl>().path() );

    // Case Meta::valTitle
    trackValue = valueForField( Meta::valTitle, trackPtr );
    QVERIFY( trackValue.toString() == trackData.value( Meta::Field::TITLE ).toString() );

    // Case Meta::valArtist for non-null artist
    trackValue = valueForField( Meta::valArtist, trackPtr );
    QVERIFY( trackValue.toString() == testArtist->name() );

    // Case Meta::valAlbum for non-null album
    trackValue = valueForField( Meta::valAlbum, trackPtr );
    QVERIFY( trackValue.toString() == testAlbum->name() );

    // Case Meta::valComposer for non-null composer
    trackValue = valueForField( Meta::valComposer, trackPtr );
    QVERIFY( trackValue.toString() == testComposer->name() );

    // Case Meta::valGenre for non-null genre
    trackValue = valueForField( Meta::valGenre, trackPtr );
    QVERIFY( trackValue.toString() == testGenre->name() );

    // Case Meta::valYear for non-null year
    trackValue = valueForField( Meta::valYear, trackPtr );
    QVERIFY( trackValue.toInt() == testYear->name().toInt() );

    // Case Meta::valComment
    trackValue = valueForField( Meta::valComment, trackPtr );
    QVERIFY( trackValue.toString() == trackData.value( Meta::Field::COMMENT ).toString() );

    // Case Meta::valTrackNr
    trackValue = valueForField( Meta::valTrackNr, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::TRACKNUMBER ).toInt() );

    // Case Meta::valDiscNr
    trackValue = valueForField( Meta::valDiscNr, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::DISCNUMBER ).toInt() );

    // Case Meta::valBpm
    trackValue = valueForField( Meta::valBpm, trackPtr );
    QVERIFY( trackValue.toDouble() == trackData.value( Meta::Field::BPM ).toDouble() );

    // Case Meta::valLength
    trackValue = valueForField( Meta::valLength, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::LENGTH ).toInt());

    // Case Meta::valBitrate
    trackValue = valueForField( Meta::valBitrate, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::BITRATE ).toInt() );

    // Case Meta::valSamplerate
    trackValue = valueForField( Meta::valSamplerate, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::SAMPLERATE ).toInt() );

    // Case Meta::valFilesize
    trackValue = valueForField( Meta::valFilesize, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::FILESIZE ).toInt() );

    // Case Meta::valFormat
    trackValue = valueForField( Meta::valFormat, trackPtr );
     QVERIFY( trackValue.toInt() == int( Amarok::FileTypeSupport::fileType(trackPtr->type() ) ) );

    // Case Meta::valCreateDate
    trackValue = valueForField( Meta::valCreateDate, trackPtr );
    QVERIFY( trackValue.toDateTime().isNull() );

    // Case Meta::valScore
    trackValue = valueForField( Meta::valScore, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::SCORE ).toInt() );

    // Case Meta::valRating
    trackValue = valueForField( Meta::valRating, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::RATING ).toInt() );

    // Case Meta::valFirstPlayed
    trackValue = valueForField( Meta::valFirstPlayed, trackPtr );
    QVERIFY( trackValue.toDateTime() == trackData.value( Meta::Field::FIRST_PLAYED ).toDateTime() );

    // Case Meta::valLastPlayed
    trackValue = valueForField( Meta::valLastPlayed, trackPtr );
    QVERIFY( trackValue.toDateTime() == trackData.value( Meta::Field::LAST_PLAYED ).toDateTime() );

    // Case Meta::valFilesize
    trackValue = valueForField( Meta::valPlaycount, trackPtr );
    QVERIFY( trackValue.toInt() == trackData.value( Meta::Field::PLAYCOUNT ).toInt() );

    // Case Meta::valUniqueId
    trackValue = valueForField( Meta::valUniqueId, trackPtr );
    QVERIFY( trackValue.toString() == trackData.value( Meta::Field::UNIQUEID ).toString() );

    // Case Meta::valTrackGain
    trackValue = valueForField( Meta::valTrackGain, trackPtr );
    QVERIFY( trackValue.toString() == QStringLiteral("track gain") );

    // Case Meta::valTrackGainPeak
    trackValue = valueForField( Meta::valTrackGainPeak, trackPtr );
    QVERIFY( trackValue.toString() == QStringLiteral("track gain peak") );

    // Case Meta::valAlbumGainGain
    trackValue = valueForField( Meta::valAlbumGain, trackPtr );
    QVERIFY( trackValue.toString() == QStringLiteral("album gain") );

    // Case Meta::valAlbumGain
    trackValue = valueForField( Meta::valAlbumGainPeak, trackPtr );
    QVERIFY( trackValue.toString() ==QStringLiteral( "album gain peak") );

    // Case Meta::valAlbumArtist
    trackValue = valueForField( Meta::valAlbumArtist, trackPtr );
    QVERIFY( trackValue.toString() == testAlbum->albumArtist()->name() );

    // Case Meta::valLabel, order of the label names remains same
    trackValue = valueForField( Meta::valLabel, trackPtr );
    QStringList labelNames;
    labelNames << testLabel1->name() << testLabel2->name() << testLabel3->name();
    QVERIFY( trackValue.toStringList() == labelNames );

    // Case Meta::valModified
    trackValue = valueForField( Meta::valModified, trackPtr );
    QVERIFY( trackValue.toDateTime().isNull() );

    // Default, returns an invalid QVariant
    trackValue = valueForField( qint64( -1 ), trackPtr );
    QVERIFY( !trackValue.isValid() );

    // Cases with null artist, album, etc. where an invalid QVariant is returned
    testAlbum->m_albumArtist = nullptr;
    testTrack->m_album = nullptr;
    testTrack->m_artist = nullptr;
    testTrack->m_composer = nullptr;
    testTrack->m_genre = nullptr;
    testTrack->m_year = nullptr;

    // Case Meta::valArtist for null artist
    trackValue = valueForField( Meta::valArtist, trackPtr );
    QVERIFY( !trackValue.isValid() );

    // Case Meta::valAlbum for null album
    trackValue = valueForField( Meta::valAlbum, trackPtr );
    QVERIFY( !trackValue.isValid() );

    // Case Meta::valComposer for null composer
    trackValue = valueForField( Meta::valComposer, trackPtr );
    QVERIFY( !trackValue.isValid() );

    // Case Meta::valGenre for null genre
    trackValue = valueForField( Meta::valGenre, trackPtr );
    QVERIFY( !trackValue.isValid() );

    // Case Meta::valYear for null year
    trackValue = valueForField( Meta::valYear, trackPtr );
    QVERIFY( !trackValue.isValid() );

    // Case Meta::valAlbumArtist for null album artist
    trackValue = valueForField( Meta::valAlbumArtist, trackPtr );
    QVERIFY( !trackValue.isValid() );
}
