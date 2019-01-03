/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef NAVIGATIONURLRUNNER_H
#define NAVIGATIONURLRUNNER_H

#include "AmarokUrl.h"
#include "AmarokUrlRunnerBase.h"

#include <QIcon>


class NavigationUrlRunner : public AmarokUrlRunnerBase
{
public:
    NavigationUrlRunner();

    virtual ~NavigationUrlRunner();

    QString command() const override;
    QString prettyCommand() const override;
    QIcon icon() const override;
    bool run( AmarokUrl url ) override;
};

#endif
