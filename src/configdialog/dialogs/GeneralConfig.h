/****************************************************************************************
 * Copyright (c) 2004-2007 Mark Kretschmann <kretschmann@kde.org>                       *
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

#ifndef GENERALCONFIG_H
#define GENERALCONFIG_H

#include "ui_GeneralConfig.h"
#include "configdialog/ConfigDialogBase.h"

class GeneralConfig : public ConfigDialogBase, public Ui_GeneralConfig
{
    Q_OBJECT

    public:
        explicit GeneralConfig( Amarok2ConfigDialog* parent );
        ~GeneralConfig() override;

        bool hasChanged() override;
        bool isDefault() override;
        void updateSettings() override;
};

#endif
