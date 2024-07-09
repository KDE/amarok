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

#include <QStringList>


class KArchiveDirectory;
class KArchiveFile;
class ScriptSelector;
class QPushButton;
class QVBoxLayout;

/**
  * A widget that allows configuration of scripts
  */
class ScriptsConfig : public ConfigDialogBase
{
    Q_OBJECT

public:
    explicit ScriptsConfig( Amarok2ConfigDialog *parent );
    ~ScriptsConfig() override;

    void updateSettings() override;
    bool hasChanged() override;
    bool isDefault() override;

public Q_SLOTS:
    void slotConfigChanged( bool changed );

private Q_SLOTS:
    void slotManageScripts();
    void installLocalScript();
    void slotReloadScriptSelector();
    void slotUpdateScripts();
    void slotUninstallScript();
    void restoreScrollBar();

private:
    const KArchiveFile *findScriptMetadataFile( const KArchiveDirectory *dir, const bool spec = false ) const;

    bool m_configChanged;
    ScriptSelector *m_selector;
    QTimer *m_timer;
    QVBoxLayout *m_verticalLayout;
    QPushButton *m_uninstallButton;
    QObject *m_parent;
    ScriptSelector *m_oldSelector;
};

#endif // AMAROK_SCRIPTSCONFIG_H
