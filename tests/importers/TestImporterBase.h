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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef TEST_IMPORTER_BASE
#define TEST_IMPORTER_BASE

#include <QExplicitlySharedDataPointer>
#include <QObject>
#include <QSet>

namespace StatSyncing
{
    class Provider;
    typedef QSharedPointer<Provider> ProviderPtr;
}

/**
 * A base class for Importer tests. It relies on convention to thoroughly test
 * a provider. It uses the provider returned by @see TestImporterBase::provider()
 * pure virtual method, which should be initialized with a database containing tracks
 * found in tests/importers/files/testcollection . Additionally, some of these tracks
 * must have their statistics set in the following way:
 *
 * title,  artist,         firstPlayed, lastPlayed, rating, playcount
 * title0, testStatistics, 1378125780,  1378125781, 1,      20
 * title1, testStatistics, 1378125782,  1378125783, 2,      15
 * title2, testStatistics, 1378125784,  1378125785, 3,      14
 * title3, testStatistics, 1378125786,  1378125787, 4,      13
 * title4, testStatistics, 1378125788,  1378125789, 5,      11
 * title5, testStatistics, 1378125790,  1378125791, 6,      10
 * title6, testStatistics, 1378125792,  1378125793, 7,       7
 * title7, testStatistics, 1378125794,  1378125795, 8,       5
 * title8, testStatistics, 1378125796,  1378125797, 9,       3
 * title9, testStatistics, 1378125798,  1378125799, 10,      2
 *
 * title,  artist,               firstPlayed, lastPlayed, rating, playcount
 * title0, testStatisticsNotSet, ,            ,            ,
 * title1, testStatisticsNotSet, ,            1378125783, 2,      15
 * title2, testStatisticsNotSet, ,            ,           3,      14
 * title3, testStatisticsNotSet, ,            1378125787,  ,      13
 * title4, testStatisticsNotSet, ,            ,           5,
 * title5, testStatisticsNotSet, 1378125790,  1378125791, 6,      10
 * title6, testStatisticsNotSet, 1378125792,  ,            ,       7
 * title7, testStatisticsNotSet, 1378125794,  1378125795, 8,       5
 * title8, testStatisticsNotSet, 1378125796,  ,           9,
 * title9, testStatisticsNotSet, 1378125798,  1378125799,  ,       2
 *
 * title,  artist,         labels
 * title0, testStatistics, 'singleTag'
 * title1, testStatistics, 'multiple', 'tags'
 * title2, testStatistics, 'caseSensitive', 'casesensitive'
 * title3, testStatistics, '☢'
 */
class TestImporterBase : public QObject
{
    Q_OBJECT

public:
    TestImporterBase();

protected:
    /**
     * This method should return provider already configured for testing.
     */
    virtual StatSyncing::ProviderPtr getProvider() = 0;

    /**
     * This method should return a provider ready for writing. The database used should
     * be a temporary copy.
     */
    virtual StatSyncing::ProviderPtr getWritableProvider();

    /**
     * Return a binary or of Meta::val* representing statistics suported
     * by the provider being tested.
     */
    virtual qint64 reliableStatistics() const = 0;

    /**
     * Returns true if the provider is capable of expressing odd-number ratings
     * (half-stars). Otherwise tests treat a half-star like a whole star (i.e.
     * a song normally with a rating 3 is assumed to have a rating 4).
     */
    virtual bool hasOddRatings() const;

private:
    void checkStatistics( const QString &artist );
    void labels( const StatSyncing::ProviderPtr &provider, const QString &trackName );

    QSet<QString> m_lbl;

private Q_SLOTS:
    void titleShouldBeCaseSensitive();
    void artistShouldBeCaseSensitive();
    void albumShouldBeCaseSensitive();
    void composerShouldBeCaseSensitive();

    void titleShouldSupportUTF();
    void artistShouldSupportUTF();
    void albumShouldSupportUTF();
    void composerShouldSupportUTF();

    void titleShouldSupportMultipleWords();
    void artistShouldSupportMultipleWords();
    void albumShouldSupportMultipleWords();
    void composerShouldSupportMultipleWords();

    void titleShouldBeWhitespaceTrimmed();
    void artistShouldBeWhitespaceTrimmed();
    void albumShouldBeWhitespaceTrimmed();
    void composerShouldBeWhitespaceTrimmed();

    void albumShouldBeUnsetIfTagIsUnset();
    void composerShouldBeUnsetIfTagIsUnset();
    void yearShouldBeUnsetIfTagIsUnset();
    void trackShouldBeUnsetIfTagIsUnset();
    void discShouldBeUnsetIfTagIsUnset();

    void tracksShouldHaveStatistics_data();
    void tracksShouldHaveStatistics();

    void tracksShouldBehaveNicelyWithNoStatistics_data();
    void tracksShouldBehaveNicelyWithNoStatistics();

    void tracksShouldWorkWithSingleLabel();
    void tracksShouldWorkWithMultipleLabels();
    void tracksShouldWorkWithCaseSensitiveLabels();
    void tracksShouldWorkWithUTFLabels();

    void providerShouldReturnNoTracksForNonexistentArtist();
    void providerShouldNotBreakOnLittleBobbyTables();

    // Write capabilities
    void commitAfterSettingAllStatisticsShouldSaveThem_data();
    void commitAfterSettingAllStatisticsShouldSaveThem();
    void commitAfterSettingFirstPlayedShouldSaveIt_data();
    void commitAfterSettingFirstPlayedShouldSaveIt();
    void commitAfterSettingLastPlayedShouldSaveIt_data();
    void commitAfterSettingLastPlayedShouldSaveIt();
    void commitAfterSettingRatingShouldSaveIt_data();
    void commitAfterSettingRatingShouldSaveIt();
    void commitAfterSettingPlaycountShouldSaveIt_data();
    void commitAfterSettingPlaycountShouldSaveIt();
    void commitAfterSettingLabelsShouldSaveThem_data();
    void commitAfterSettingLabelsShouldSaveThem();
};

#endif // TEST_IMPORTER_BASE
