// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_PLUGINCONFIG_H
#define AMAROK_PLUGINCONFIG_H

#include <qobject.h>

class QWidget;

namespace Amarok
{
    /**
     * Class to allow user configuration of your plugin; you provide a GUI widget via view()
     */

    class PluginConfig : public QObject
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

