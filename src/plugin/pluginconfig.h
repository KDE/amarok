// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_PLUGINCONFIG_H
#define AMAROK_PLUGINCONFIG_H

#include <qobject.h>

class QWidget;

namespace amaroK
{
    /**
     * Class to allow user configuration of your plugin; you provide a GUI widget via view()
     */

    class PluginConfig : public QObject
    {
        Q_OBJECT

        signals:
            /** Emit whenever some view setting is changed by the user */
            //TODO this is wrongly named, instead name it doUpdateButtons()
            //     or something better, settingsChanged has a different meaning in KConfigDialog
            void settingsChanged();

        public:
            /** Return the view widget */
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

