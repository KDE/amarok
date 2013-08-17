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

#ifndef STATSYNCING_SIMPLE_WRITABLE_TRACK_H
#define STATSYNCING_SIMPLE_WRITABLE_TRACK_H

#include "SimpleTrack.h"

#include <QReadWriteLock>

namespace StatSyncing {

/**
 * A simple implementation of @see StatSyncing::Track which wraps data contained
 * in Meta::FieldHash @param metadata container, and for every function returns value
 * corresponding to an adequate Meta::val* key. Labels are passed through @param labels .
 *
 * This class is suitable for read-write operations. If you only need read capabilities,
 * consider using @see StatSyncing::SimpleTrack .
 */
class AMAROK_EXPORT SimpleWritableTrack : public SimpleTrack
{
public:
    explicit SimpleWritableTrack( const Meta::FieldHash &metadata,
                                  const QSet<QString> &labels = QSet<QString>() );
    virtual ~SimpleWritableTrack();

    virtual QDateTime firstPlayed() const;
    virtual void setFirstPlayed( const QDateTime &firstPlayed );

    virtual QDateTime lastPlayed() const;
    virtual void setLastPlayed( const QDateTime &lastPlayed );

    virtual int playCount() const;
    virtual void setPlayCount( int playCount );

    virtual int rating() const;
    virtual void setRating( int rating );

    virtual QSet<QString> labels() const;
    virtual void setLabels( const QSet<QString> &labels );

    void commit();

protected:
    /**
     * You must reimplement this method to save statistics to the database.
     * @param changes holds fields changed since last doCommit() call.
     * Note that the changes already are visible in the track through getter methods.
     * Label changes will be indicated by Meta::valLabel field.
     *
     * Also note that m_changeLock will already be write-locked when this method
     * is called.
     */
    virtual void doCommit( const QSet<qint64> &fields ) = 0;

    Meta::FieldHash m_statistics;

    /**
     * You must read-lock lock before reading, and write-lock before
     * writing, m_labels, m_metadata and m_statistics members.
     */
    mutable QReadWriteLock m_lock;

private:
    QSet<qint64> m_changes;
};

} // namespace StatSyncing

#endif // STATSYNCING_SIMPLE_WRITABLE_TRACK_H
