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

#ifndef STATSYNCING_OPTIONS_H
#define STATSYNCING_OPTIONS_H

#include <QtGlobal>
#include <QString>
#include <QSet>

namespace StatSyncing
{
    /**
     * A container for options controlling how synchronization of two tracks is
     * performed.
     */
    class Options
    {
        public:
            explicit Options();

            /**
             * Binary OR of Meta::val* fields that should be synced.
             */
            qint64 syncedFields() const;
            void setSyncedFields( qint64 fields );

            /**
             * A list of labels statsyncing should not touch at all.
             */
            QSet<QString> excludedLabels() const;
            void setExcludedLabels( const QSet<QString> &labels );

        private:
            qint64 m_syncedFields;
            QSet<QString> m_excludedLabels;
    };

} // namespace StatSyncing

#endif // STATSYNCING_OPTIONS_H
