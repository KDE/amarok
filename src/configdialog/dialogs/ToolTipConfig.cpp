/****************************************************************************************
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                           *
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

#include "ToolTipConfig.h"

#include "amarokconfig.h"
#include "Amarok.h"
#include "ActionClasses.h"
#include "EngineController.h"
#include "Debug.h"

#include <KCMultiDialog>
#include <kmessagebox.h>


ToolTipConfig::ToolTipConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this );
    bool toolTipEnabled = kcfg_enableToolTips->isChecked();
    kcfg_ToolTips_Album->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Artist->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Comment->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Composer->setEnabled(toolTipEnabled);
    kcfg_ToolTips_DiskNumber->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Genre->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Title->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Track->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Year->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Location->setEnabled(toolTipEnabled);
    kcfg_ToolTips_Artwork->setEnabled(toolTipEnabled);
}

ToolTipConfig::~ToolTipConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
ToolTipConfig::hasChanged()
{
    return false;
}

bool
ToolTipConfig::isDefault()
{
    return false;
}

void
ToolTipConfig::updateSettings()
{}
#include "ToolTipConfig.moc"
