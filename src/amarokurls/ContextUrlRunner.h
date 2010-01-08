/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/


#ifndef CONTEXTURLRUNNER_H
#define CONTEXTURLRUNNER_H

#include "amarok_export.h"
#include "AmarokUrlRunnerBase.h"

class AmarokUrl;
class KIcon;
class QString;


class AMAROK_EXPORT ContextUrlRunner : public AmarokUrlRunnerBase
{
public:
    ContextUrlRunner();
    ~ContextUrlRunner();
    
    virtual KIcon icon() const;
    virtual bool run(AmarokUrl url);
    virtual QString command() const;
};

#endif // CONTEXTURLRUNNER_H
