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

#ifndef STATSYNCING_SIMPLE_TRACK_H
#define STATSYNCING_SIMPLE_TRACK_H

#include "Track.h"

#include "MetaValues.h"

namespace StatSyncing {

/**
 * A simple implementation of @see StatSyncing::Track which wraps data contained
 * in Meta::FieldHash @param metadata container, and for every function returns value
 * corresponding to an adequate Meta::val* key. Labels are passed through @param labels .
 *
 * This class is thread safe only for reading operations. If you need read-write access,
 * @see StatSyncing::SimpleWritableTrack can be used.
 */
class AMAROK_EXPORT SimpleTrack : public Track
{
public:
    explicit SimpleTrack( const Meta::FieldHash &metadata,
                          const QSet<QString> &labels = QSet<QString>() );
    ~SimpleTrack() override;

    QString name() const override;
    QString album() const override;
    QString artist() const override;
    QString composer() const override;
    int year() const override;
    int trackNumber() const override;
    int discNumber() const override;

    QDateTime firstPlayed() const override;
    QDateTime lastPlayed() const override;
    int playCount() const override;
    int rating() const override;

    QSet<QString> labels() const override;

protected:
    QDateTime getDateTime( const QVariant &v ) const;

    QSet<QString> m_labels;
    Meta::FieldHash m_metadata;
};

} // namespace StatSyncing

#endif // STATSYNCING_SIMPLE_TRACK_H
