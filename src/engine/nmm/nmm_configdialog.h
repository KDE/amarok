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

class NmmConfigDialog : public amaroK::PluginConfig
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

class NmmLocation {
  public:
    NmmLocation();
    NmmLocation(QString hostname, bool audio, bool video, int volume);
    ~NmmLocation();

    QString hostname() const;
    void setHostname(QString);

    bool audio() const { return m_audio; }
    void setAudio( bool audio ) { m_audio = audio; }

    bool video() const { return m_video; } 
    void setVideo( bool video ) { m_video = video; }

  private:
    QString m_hostname;
    bool m_audio;
    bool m_video;
    int m_volume;
};

#endif
