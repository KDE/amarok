/****************************************************************************************
 * Copyright (c) 2004-2007 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2005 Ian Monroe <ian@monroe.nu>                                        *
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

#include "GeneralConfig.h"

GeneralConfig::GeneralConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this );
}

GeneralConfig::~GeneralConfig()
{}

///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
GeneralConfig::hasChanged()
{
    return false;
}

bool
GeneralConfig::isDefault()
{
    return false;
}

void
GeneralConfig::updateSettings() //SLOT
{
}

#include "GeneralConfig.moc"
