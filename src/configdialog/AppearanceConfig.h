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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef APPEARANCECONFIG_H
#define APPEARANCECONFIG_H

#include "ui_AppearanceConfig.h"
#include "ConfigDialogBase.h"


class AppearanceConfig : public ConfigDialogBase
{
    public:
        AppearanceConfig( QWidget* parent );
        virtual ~AppearanceConfig();

        virtual bool hasChanged();
        virtual bool isDefault();
        virtual void updateSettings();

    private:
        Ui_AppearanceConfig* m_gui;
};


#endif

