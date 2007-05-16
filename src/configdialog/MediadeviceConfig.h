/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
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

#ifndef MEDIADEVICECONFIG_H
#define MEDIADEVICECONFIG_H

#include "ConfigDialogBase.h"

class MediumPluginManager;


class MediadeviceConfig : public ConfigDialogBase
{
    Q_OBJECT

    public:
        MediadeviceConfig( QWidget* parent );
        virtual ~MediadeviceConfig();

        virtual bool hasChanged();
        virtual bool isDefault();
        virtual void updateSettings();

    private:
        MediumPluginManager *m_deviceManager;
};


#endif


