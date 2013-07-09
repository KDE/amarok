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

#include "statsyncing/Provider.h"

#include "MetaValues.h"
#include "FastForwardConfig.h"

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QMutex>

namespace StatSyncing
{

class FastForwardTrack : public QObject, public Track
{
    Q_OBJECT

public:
    FastForwardTrack( const QString &trackUrl, const QString &providerUid );
    virtual ~FastForwardTrack();

    QString name() const;
    QString album() const;
    QString artist() const;
    QString composer() const;
    int year() const;
    int trackNumber() const;
    int discNumber() const;

    int rating() const;
    QDateTime firstPlayed() const;
    QDateTime lastPlayed() const;
    int playCount() const;
    QSet<QString> labels() const;

private:
    void checkAllDataRetrieved() const;

    QString m_trackUrl;
    QString m_providerUid;

    QMap<quint64, QVariant> m_metadata;
    QMap<quint64, QVariant> m_statistics;
    QSet<QString> m_labels;

    mutable QMutex m_statMutex;

private slots:
    void retrievePersonalData();
};

} // namespace StatSyncing

#endif // STATSYNCING_FAST_FORWARD_TRACK_H
