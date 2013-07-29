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

#ifndef STATSYNCING_RHYTHMBOX_TRACK_H
#define STATSYNCING_RHYTHMBOX_TRACK_H

#include "statsyncing/Provider.h"

#include <QObject>

namespace StatSyncing
{

class RhythmboxTrack : public QObject, public Track
{
    Q_OBJECT

public:
    RhythmboxTrack( const QMap<qint64, QString> &metadata );
    ~RhythmboxTrack();

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
    QMap<qint64, QString> m_metadata;
};

} // namespace StatSyncing

#endif // STATSYNCING_RHYTHMBOX_TRACK_H
