// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_PLUGINCONFIG_H
#define AMAROK_PLUGINCONFIG_H

#include <qobject.h>

class QWidget;

namespace amaroK
{
    /**
     * Interface class for plugin configuration GUIs
     */ 
    class PluginConfig : public QObject
    {
        Q_OBJECT
        
        signals:
            /** Emitted when view was modified */
            void settingsChanged();
        
        public:
            /** Return the configuration GUI */
            virtual QWidget* view() const = 0;
            
            /** Return whether the view was modified */
            virtual bool hasChanged() const = 0;
            
            /** Return whether all view settings are in default state */
            virtual bool isDefault() const = 0;

        public slots:                
            /** Save view state into configuration */
            virtual void save() = 0;
    };
}


#endif /*AMAROK_PLUGINCONFIG_H*/

