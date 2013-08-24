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

#ifndef STATSYNCING_FAST_FORWARD_TRACK_H
#define STATSYNCING_FAST_FORWARD_TRACK_H

#include "statsyncing/SimpleTrack.h"

#include <QMutex>

namespace StatSyncing
{

class FastForwardTrack : public QObject, public SimpleTrack
{
    Q_OBJECT

public:
    FastForwardTrack( const Meta::FieldHash &metadata, const QString &trackUrl,
                      const QString &providerUid );
    ~FastForwardTrack();

    QDateTime firstPlayed() const;
    QDateTime lastPlayed() const;
    int rating() const;
    int playCount() const;
    QSet<QString> labels() const;

private:
    void assureStatisticsRetrieved() const;

    const QString m_providerUid;
    Meta::FieldHash m_statistics;
    bool m_statisticsRetrieved;
    const QString m_trackUrl;

    mutable QMutex m_mutex;

private slots:
    void retrieveStatistics();
};

} // namespace StatSyncing

#endif // STATSYNCING_FAST_FORWARD_TRACK_H
