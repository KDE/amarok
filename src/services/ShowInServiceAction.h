/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#ifndef SHOWINSERVICEACTION_H
#define SHOWINSERVICEACTION_H

#include "ServiceBase.h"
#include "ServiceMetaBase.h"

#include <QAction>

/**
An action to let the user locate and show the artist or album of a track in the correct service

    @author
*/
class AMAROK_EXPORT ShowInServiceAction : public QAction
{
    Q_OBJECT
public:
    ShowInServiceAction( ServiceBase * service, Meta::ServiceTrack *track );

    ~ShowInServiceAction();

private slots:
    void slotTriggered();

private:
    Meta::ServiceTrack * m_track;
    ServiceBase * m_service;
};

#endif
