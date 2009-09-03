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

#ifndef PERMANENTURLSTATISTICSPROVIDER_H
#define PERMANENTURLSTATISTICSPROVIDER_H

#include "meta/StatisticsProvider.h"

#include "amarok_export.h"

#include <QString>

class AMAROK_EXPORT PermanentUrlStatisticsProvider : public Meta::StatisticsProvider
{
public:
    PermanentUrlStatisticsProvider( const QString &permanentUrl );

protected:
    virtual void save();

private:
    QString m_permanentUrl;
};

#endif // PERMANENTURLSTATISTICSPROVIDER_H
