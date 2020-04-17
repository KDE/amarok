/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                     *
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

#ifndef WEEKLY_TOP_BIAS_H
#define WEEKLY_TOP_BIAS_H

#include "dynamic/biases/TagMatchBias.h"

class QNetworkReply;

namespace Dynamic
{

    /**
     *  This is a bias which allows the user to select a range of dates, and then
     *  adds to the playlist tracks/artists/albums that were on their last.fm top
     *  tracks during that time
     *
     */
    class WeeklyTopBias : public SimpleMatchBias
    {
        Q_OBJECT

        public:
            struct DateRange
            {
                QDateTime from;
                QDateTime to;
            };

            WeeklyTopBias();
            ~WeeklyTopBias() override;

            void fromXml( QXmlStreamReader *reader ) override;
            void toXml( QXmlStreamWriter *writer ) const override;

            static QString sName();
            QString name() const override;
            QString toString() const override;

            QWidget* widget( QWidget* parent = nullptr ) override;

            bool trackMatches( int position,
                               const Meta::TrackList& playlist,
                               int contextCount ) const override;


            DateRange range() const;
            void setRange( const DateRange &range );

        private Q_SLOTS:
            void newQuery() override;
            void newWeeklyTimesQuery();
            void newWeeklyArtistQuery();

            void weeklyTimesQueryFinished();
            void weeklyArtistQueryFinished();

            void fromDateChanged( const QDateTime& );
            void toDateChanged( const QDateTime& );

        private:
            void loadFromFile();
            void saveDataToFile() const;

            DateRange m_range;

            // be able to warn the user
            uint m_earliestDate;

            QList< uint > m_weeklyFromTimes;
            QList< uint > m_weeklyToTimes;
            QHash< uint, QStringList > m_weeklyArtistMap;

            QNetworkReply* m_weeklyTimesJob;
            QHash< uint, QNetworkReply*> m_weeklyArtistJobs;
    };

    class WeeklyTopBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            QString i18nName() const override;
            QString name() const override;
            QString i18nDescription() const override;
            BiasPtr createBias() override;
    };
}

#endif
