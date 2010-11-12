/****************************************************************************************
 * Copyright (c) 2010 Kevin Funk <krf@electrostorm.net>                                 *
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

#ifndef DEBUGPRIVATE_H
#define DEBUGPRIVATE_H

#include "Debug.h"

#include <QString>

#if QT_VERSION >= 0x040700
# include <QElapsedTimer>
#else
# include <QTime>
#endif

class IndentPrivate
    : public QObject
{
private:
    explicit IndentPrivate(QObject* parent = 0);

public:
    static IndentPrivate* instance();

    QString m_string;
};

class BlockPrivate
{
public:
    BlockPrivate( const char *text );

#if QT_VERSION >= 0x040700
    QElapsedTimer startTime;
#else
    QTime startTime;
#endif
    const char *label;
    int color;
};

#endif // DEBUGPRIVATE_H
