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
         * Selects a HostListItem as selected item.
         */
        void selectHostListItem( HostListItem* );

        void setCheckedList( bool );

    private:
        void readConfig();
        void addHostListItem( QString );
        
        /** 
         * Returns all locations in the host list.
         */
        QStringList hostList() const;

        /**
         * Returns audio toggle states for every host.
         */
        QStringList audioHostList() const;

        /**
         * Returns video toggle sattes for every host.
         */
        QStringList videoHostList() const;

        NmmConfigDialogBase* m_view;

        QWidget* audio_vbox;;

        HostListItem *current_host;
        QPtrList<HostListItem> m_hosts;
};

#endif
