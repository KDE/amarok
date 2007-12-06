/***************************************************************************
 *   Copyright (c) 2007  Jamie Faris <farisj@gmail.com>                    *
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
#ifndef COPYTODEVICEACTION_H
#define COPYTODEVICEACTION_H

#include <QAction>

#include "Meta.h"

/**
 * This action adds the track to the MediaBrowser transfer queue.
 *
 * @author Jamie Faris
 */
class AMAROK_EXPORT CopyToDeviceAction : public QAction
{
    Q_OBJECT
    public:
        CopyToDeviceAction( QObject* parent, Meta::Track *track );

    private slots:
        void slotTriggered();

    private:
        Meta::TrackPtr m_track;
};

#endif //COPYTODEVICEACTION_H
