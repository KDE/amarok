/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_SCRIPTSCONFIG_H
#define AMAROK_SCRIPTSCONFIG_H

#include "configdialog/ConfigDialogBase.h"

class ScriptSelector;

/**
  * A widget that allows configuration of scripts
  */
class ScriptsConfig : public ConfigDialogBase
{
    Q_OBJECT

public:
    ScriptsConfig( QWidget *parent );
    virtual ~ScriptsConfig();

    virtual void updateSettings();
    virtual bool hasChanged();
    virtual bool isDefault();

public slots:
    void slotConfigChanged( bool changed );

private slots:
    void slotManageScripts();
    void installLocalScript();
    void slotReloadScriptSelector();
    void slotUpdateScripts();

private:
    bool m_configChanged;
    ScriptSelector *m_selector;
    QTimer *m_timer;
};

#endif // AMAROK_SCRIPTSCONFIG_H
