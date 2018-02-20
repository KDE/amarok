/*
    Copyright (c) 2010 Kevin Funk <krf@electrostorm.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DEBUGPRIVATE_H
#define DEBUGPRIVATE_H

#include "Debug.h"

#include <QIODevice>
#include <QString>

class IndentPrivate
    : public QObject
{
private:
    explicit IndentPrivate(QObject* parent = 0);

public:
    static IndentPrivate* instance();

    QString m_string;
};

class NoDebugStream : public QIODevice
{
public:
    NoDebugStream()
    {
        open(WriteOnly);
    }
    bool isSequential() const Q_DECL_OVERRIDE
    {
        return true;
    }
    qint64 readData(char *, qint64) Q_DECL_OVERRIDE
    {
        return 0;
    }
    qint64 readLineData(char *, qint64) Q_DECL_OVERRIDE
    {
        return 0;
    }
    qint64 writeData(const char *, qint64 len) Q_DECL_OVERRIDE
    {
        return len;
    }
};

#endif // DEBUGPRIVATE_H
