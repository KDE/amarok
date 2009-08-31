/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef AMAROK_PLUGINCONFIG_H
#define AMAROK_PLUGINCONFIG_H
#include "amarok_export.h"
#include <QObject>

class QWidget;

namespace Amarok
{
    /**
     * Class to allow user configuration of your plugin; you provide a GUI widget via view()
     */

    class AMAROK_EXPORT PluginConfig : public QObject
    {
        Q_OBJECT

        signals:
            /** Emit whenever some view setting is changed by the user */
            void viewChanged();

            /** Emit after settings have been saved to config. Can be used for updating engine state. */
            void settingsSaved();

        public:
            /** Return the view widget,
              * The PluginConfig object owns this pointer, nobody else will delete it for you
              */
            virtual QWidget* view() = 0;

            /** Return true if any of the view settings are different to the currently saved state */
            virtual bool hasChanged() const = 0;

            /** Return true if all view settings are in their default states */
            virtual bool isDefault() const = 0;

        public slots:
            /** Save view state using, eg KConfig */
            virtual void save() = 0;
    };
}

#endif /*AMAROK_PLUGINCONFIG_H*/

