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
     * Adds a host to the location list.
     */
    void addHost();

    /**
     * Removes a host from the location list.
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

  private:
    /**
     * Creates a host item in the host list.
     * TODO: more docu
     */
    void addHostListItem( QString hostname, bool audio = true, bool video = true, int volume = 0, bool read_only = false );

    /**
     * Fills host list with HostListItems by reading
     * tmp_environment_list or tmp_user_list.
     */
    void createHostList( bool use_environment_list = false );

    /** 
     * Returns all locations in the host list.
     */
    QStringList hostList() const;

    /**
     * Clears current host list.
     * \param save_user_hostlist saves user host list if true
     */
    void removeHostList( bool save_user_hostlist = false );

    /**
     * Saves user host list to tmp_user_list.
     *
     * Used on audioGroup change and NmmConfigDialog::save().
     */
    void saveUserHostList();
    
    NmmConfigDialogBase* m_view;

    /**
     * Currently selected HostListItem.
     * NULL if none selected.
     */
    HostListItem *current_host;

    typedef QValueList<NmmLocation> NmmLocationList;

    /**
     * Environment host list.
     * Populated only once in constructor.
     */
    NmmLocationList tmp_environment_list;

    /**
     *
     */
    NmmLocationList tmp_user_list;

    /**
     * Current audio group selection.
     */
    int current_audio_group_selection;
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
