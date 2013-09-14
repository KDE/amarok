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

#ifndef STATSYNCING_ITUNES_TRACK_H
#define STATSYNCING_ITUNES_TRACK_H

#include "statsyncing/SimpleWritableTrack.h"

#include <QObject>

namespace StatSyncing
{

class ITunesTrack : public QObject, public SimpleWritableTrack
{
    Q_OBJECT

public:
    explicit ITunesTrack( const int trackId, const Meta::FieldHash &metadata );
    ~ITunesTrack();

    int rating() const;
    void setRating( int rating );
    QDateTime lastPlayed() const;

signals:
    void commitCalled( const int trackId, const Meta::FieldHash &statistics );

protected:
    void doCommit( const qint64 changes );

private:
    const int m_trackId;
};

} // namespace StatSyncing

#endif // STATSYNCING_ITUNES_TRACK_H
