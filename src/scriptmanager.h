// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#include "config.h"
#ifdef HAVE_KJSEMBED

#ifndef AMAROK_SCRIPTMANAGER_H
#define AMAROK_SCRIPTMANAGER_H

#include "scriptmanager_selector.h"

#include <qobject.h>            //baseclass
#include <qstringlist.h>        //stack allocated

namespace KJSEmbed
{
    class KJSEmbedPart;
}

namespace ScriptManager
{

    class Manager : public QObject
    {
            Q_OBJECT

        signals:
            void stop( const QString& );
            void configure( const QString& );

        public:
            Manager( QObject* );

            void showSelector();
            void showConsole();
            void addObject( QObject* object );

            //static
            static Manager *instance() { return s_instance; }

        private slots:
            void slotEdit( const QString& );
            void slotRun( const QString& );
            void slotStop( const QString& );
            void slotConfigure( const QString& );

        private:
            static Manager* s_instance;

            KJSEmbed::KJSEmbedPart* m_kjs;
            QStringList m_list;
    };

} //namespace ScriptManager

#endif /* AMAROK_SCRIPTMANAGER_H */

#endif /*HAVE_KJSEMBED*/

