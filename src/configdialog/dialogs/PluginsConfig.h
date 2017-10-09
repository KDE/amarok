/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROK_PLUGINSCONFIG_H
#define AMAROK_PLUGINSCONFIG_H

#include "configdialog/ConfigDialogBase.h"
#include <KLocalizedString>

class KPluginSelector;

/**
  * A widget that allows configuration of plugins
  */
class PluginsConfig : public ConfigDialogBase
{
    Q_OBJECT

public:
    PluginsConfig( Amarok2ConfigDialog *parent );
    virtual ~PluginsConfig();

    void updateSettings() Q_DECL_OVERRIDE;
    bool hasChanged() Q_DECL_OVERRIDE;
    bool isDefault() Q_DECL_OVERRIDE;

public Q_SLOTS:
    void slotConfigChanged( bool changed );

private:
    bool m_configChanged;
    KPluginSelector *m_selector;
};

#endif // AMAROK_PLUGINSCONFIG_H
