/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#ifndef SERVICESOURCEINFOCAPABILITY_H
#define SERVICESOURCEINFOCAPABILITY_H

#include "amarok_export.h"
#include "meta/SourceInfoCapability.h"


class SourceInfoProvider;

/**
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/

class AMAROK_EXPORT ServiceSourceInfoCapability : public Meta::SourceInfoCapability
{
public:
    ServiceSourceInfoCapability( SourceInfoProvider * sourceInfoProvider );

    ~ServiceSourceInfoCapability();

    QString sourceName();
    QString sourceDescription();
    QPixmap emblem();

private:
    SourceInfoProvider * m_sourceInfoProvider;

};

#endif
