/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef LASTFM_WS_REQUEST_PARAMETERS_H
#define LASTFM_WS_REQUEST_PARAMETERS_H

#include <lastfm/DllExportMacro.h>
#include <QString>
#include <QList>
#include <QPair>
#include <QMap>


class WsRequestParameters
{
public:
    WsRequestParameters();

    void add( const QString& key, const QString& value ) { m_map[key] = value; }
    operator const QList<QPair<QString, QString> >() const;

private:
    QString methodSignature() const;
    QMap<QString, QString> m_map;
};

#endif
