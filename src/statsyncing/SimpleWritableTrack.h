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
    ~SimpleWritableTrack() override;

    QDateTime firstPlayed() const override;

    /**
     * Sets the First Played statistic. This method saves @param firstPlayed in the form
     * of unix timestamp.
     */
    void setFirstPlayed( const QDateTime &firstPlayed ) override;

    QDateTime lastPlayed() const override;

    /**
     * Sets the Last Played statistic. This method saves @param lastPlayed in the form
     * of unix timestamp.
     */
    void setLastPlayed( const QDateTime &lastPlayed ) override;

    int playCount() const override;
    void setPlayCount( int playCount ) override;

    int rating() const override;
    void setRating( int rating ) override;

    QSet<QString> labels() const override;
    void setLabels( const QSet<QString> &labels ) override;

    void commit() override;

protected:
    /**
     * You must reimplement this method to save statistics to the database.
     * @param changes holds a bitmask of fields changed since last doCommit() call.
     * Note that the changes already are visible in the track through getter methods.
     * Label changes will be indicated by Meta::valLabel field.
     *
     * Also note that m_changeLock will already be write-locked when this method
     * is called.
     */
    virtual void doCommit( const qint64 changes ) = 0;

    Meta::FieldHash m_statistics;

    /**
     * You must read-lock lock before reading, and write-lock before
     * writing, m_labels, m_metadata and m_statistics members.
     */
    mutable QReadWriteLock m_lock;

    /**
     * A bitmask containing changed fields. Only modify this in set* methods, and only
     * using bitwise-OR.
     */
    qint64 m_changes;
};

} // namespace StatSyncing

#endif // STATSYNCING_SIMPLE_WRITABLE_TRACK_H
