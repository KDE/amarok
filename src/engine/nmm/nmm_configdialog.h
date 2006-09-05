/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2005-2006
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */


#ifndef NMMCONFIGDIALOG_H
#define NMMCONFIGDIALOG_H

#include "nmm_configdialogbase.h"
#include "plugin/pluginconfig.h"

#include "qobject.h"

class HostList;
class HostListItem;
class NmmLocation;

class NmmConfigDialog : public Amarok::PluginConfig
{
  Q_OBJECT

  public:
    NmmConfigDialog();
    ~NmmConfigDialog();

    QWidget* view() { return m_view; }

    // \todo doesn't work the intended way
    bool hasChanged() const;

    bool isDefault() const;

  public slots:

    void save();

    /**
     * Adds a host to the user list.
     */
    void addHost();

    /**
     * Removes current selected host entry from user list.
     */
    void removeHost();

    /**
     * Called when a radio button in audioGroup was clicked.
     */
    void clickedAudioGroup( int );

    /**
     * Updates status column for m_user_list and m_environment_list
     * to reflect that an error occurred for a host.
     * \param hostname host the error is related to
     * \param error error identification, see NMMEngineException::Error
     */
    void notifyHostError( QString hostname, int error );

  private slots:
    /**
     * Enables 'Remove ' host button if a HostListItem is selected.
     */
    void enableRemoveButton();

    /**
     * Called when user host list gets modified.
     * So either a host entry has been deleted/added
     * or the audio/video toggle has changed.
     */
    void hostListModified();

  private:
    /**
     * Fills user and environment host on config dialog init.
     */
    void createHostLists();

    /**
     * Designer ui configuration dialog.
     */
    NmmConfigDialogBase* m_view;

    /**
     * Current audio group selection.
     */
    int current_audio_group_selection;

    /**
     * Host list showing read-only environment list.
     */
    HostList *m_environment_list;

    /**
     * Host list create by the user.
     */
    HostList *m_user_list;

    /**
     * True if user host list was modified.
     */
    bool m_host_list_modified;
};

#endif
