/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef STATSYNCING_COMMONMODEL_H
#define STATSYNCING_COMMONMODEL_H

#include "statsyncing/Options.h"
#include "statsyncing/Track.h"

#include <QList>
#include <QVariant>
#include <QFont>

namespace StatSyncing
{
    /**
     * Helper class for {Matched,Single}TracksModel's to avoid code duplication
     */
    class CommonModel
    {
        public:
            enum {
                ResizeModeRole = Qt::UserRole,
                FieldRole,
                UserRole
            };
            static const QSize s_ratingSize;

            explicit CommonModel( const QList<qint64> &columns, const Options &options );

            QVariant headerData( int section, Qt::Orientation orientation,
                                 int role = Qt::DisplayRole ) const;

        protected:
            QVariant sizeHintData( qint64 field ) const;
            QVariant textAlignmentData( qint64 field ) const;

            QVariant trackData( const TrackPtr &track, qint64 field, int role ) const;
            QVariant trackTitleData( const TrackPtr &track ) const;
            QVariant trackToolTipData( const TrackPtr &track ) const;

            QList<qint64> m_columns;
            Options m_options;
            QFont m_normalFont;
            QFont m_boldFont;
    };

} // namespace StatSyncing

#endif // STATSYNCING_COMMONMODEL_H
