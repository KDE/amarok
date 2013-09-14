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

#ifndef STATSYNCING_CLEMENTINE_TRACK_H
#define STATSYNCING_CLEMENTINE_TRACK_H

#include "statsyncing/SimpleWritableTrack.h"

namespace StatSyncing
{

class ImporterSqlConnection;
typedef QSharedPointer<ImporterSqlConnection> ImporterSqlConnectionPtr;

class ClementineTrack : public SimpleWritableTrack
{
public:
    ClementineTrack( const QVariant &filename, const ImporterSqlConnectionPtr &connection,
                     const Meta::FieldHash &metadata );
    ~ClementineTrack();

    int year() const;
    int trackNumber() const;
    int discNumber() const;

    QDateTime lastPlayed() const;
    void setLastPlayed( const QDateTime &lastPlayed );
    int playCount() const;
    void setPlayCount( int playCount );
    int rating() const;
    void setRating( int rating );

protected:
    void doCommit( const qint64 fields );

private:
    const ImporterSqlConnectionPtr m_connection;
    const QVariant m_filename;
};

}

#endif // STATSYNCING_CLEMENTINE_TRACK_H
