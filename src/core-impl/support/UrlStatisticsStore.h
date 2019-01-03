/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef URLSTATISTICSSTORE_H
#define URLSTATISTICSSTORE_H

#include "core-impl/support/PersistentStatisticsStore.h"

#include <QString>

class AMAROK_EXPORT UrlStatisticsStore : public PersistentStatisticsStore
{
    public:
        /**
         * Construct persistent per-url statistics store. If @param permanentUrl is not
         * specified, track->uidUrl() is used.
         */
        explicit UrlStatisticsStore( Meta::Track *track, const QString &permanentUrl = QString() );

    protected:
        void save() override;

    private:
        QString m_permanentUrl;
};

#endif // URLSTATISTICSSTORE_H
