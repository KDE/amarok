/****************************************************************************************
 * Copyright (c) 2010 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_APPLICATIONCONTROLLER_H
#define AMAROK_APPLICATIONCONTROLLER_H

#include <QObject>

namespace Amarok
{

class ApplicationController : public QObject
{
    Q_OBJECT
public:
    explicit ApplicationController( QObject *parent ) : QObject( parent ) {}
    ~ ApplicationController() override {}

public Q_SLOTS:
    virtual void start() = 0;
    virtual void shutdown() = 0;
};

}

#endif
