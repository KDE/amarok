/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2005
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

class AudioHostListItem;

class NmmConfigDialog : public amaroK::PluginConfig
{
Q_OBJECT
    public:
        NmmConfigDialog();
        ~NmmConfigDialog();
        
        QWidget* view() { return m_view; }

        bool hasChanged() const;
        bool isDefault() const;
        
    public slots:
        
        void save();

        /**
         * Adds a host to the audio locations.
         */
        void addAudioHost();

        /**
         * Removes a host from the audio locations.
         */
        void removeAudioHost();

        void selectHostListItem( AudioHostListItem* );

        void addVideoHost();
        void removeVideoHost();

        void setCheckedAudioList( bool );
        void setCheckedVideoList( bool );

    private:
        void readConfig();
        void addAudioHostListItem( QString );
        
        /**
         * Returns hosts in the audio host list.
         */
        QStringList audioHostList() const;

        /**
         * Returns hosts in the video host list.
         */
        QStringList videoHostList() const;

        NmmConfigDialogBase* m_view;

        QWidget* audio_vbox;;

        AudioHostListItem *current_audio_host;
        QPtrList<AudioHostListItem> m_audio_hosts;
};

#endif
