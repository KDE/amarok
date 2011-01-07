/****************************************************************************************
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "dynamic/Bias.h"
#include "dynamic/BiasFactory.h"
#include "dynamic/biases/TagMatchBias.h"

#include <QMutex>
#include <QNetworkReply>

class KJob;

#include <QQueue>

class QSignalMapper;
class QByteArray;
class QDate;
class QDateTimeEdit;
class QVBoxLayout;
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
            explicit WeeklyTopBias( uint from = 0, uint to = 0 );
            ~WeeklyTopBias();

        Q_SIGNALS:
            void doneFetching();

        private Q_SLOTS:
            void fromDateChanged( const QDateTime& );
            void toDateChanged( const QDateTime& );

            void updateDB();
            void saveDataToFile();
            void rangeJobFinished();

            void weeklyFetch( QObject* );
            void fetchWeeklyData(uint from = 0, uint to = 0);

            // querymaker
            void updateReady( QString, QStringList );

        private:
            void getPossibleRange();
            void update();
            void fetchNextWeeks( int num = 5 );

            QSet< QByteArray > m_trackList;

            QVBoxLayout* m_layout;
            QDateTimeEdit* m_fromEdit;
            QDateTimeEdit* m_toEdit;

            QList< uint > m_weeklyCharts;
            QList< uint > m_weeklyChartsTo;
            QHash< uint, QStringList > m_weeklyChartData;
            QStringList m_currentArtistList;

            uint m_fromDate;
            uint m_toDate;

            // be able to warn the user
            uint m_earliestDate;
            QQueue< QMap<QString,QString> > m_fetchQueue;
            QSignalMapper* m_fetching;

            QNetworkReply* m_rangeJob;
            QNetworkReply* m_dataJob;
    };

    class WeeklyTopBiasFactory : public Dynamic::AbstractBiasFactory
    {
        public:
            virtual QString i18nName() const;
            virtual QString name() const;
            virtual QString i18nDescription() const;
            virtual BiasPtr createBias();
            virtual BiasPtr createBias( QXmlStreamReader *reader );
    };
}

#endif
