// (c) 2003 Scott Wheeler <wheeler@kde.org>,
// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_SCRIPTMANAGER_H
#define AMAROK_SCRIPTMANAGER_H

#include <qmap.h>

#include <kdialogbase.h>    //baseclass
#include <kurl.h>

class ScriptManagerBase;
class KRun;

class ScriptManager : public KDialogBase
{
        Q_OBJECT

    public:
        ScriptManager( QWidget *parent = 0, const char *name = 0 );
        virtual ~ScriptManager();

        static ScriptManager* instance() { return s_instance ? s_instance : new ScriptManager(); }

    public slots:

    private slots:
        void slotAddScript();
        void slotRemoveScript();
        void slotEditScript();
        void slotRunScript();
        void slotStopScript();
        void slotConfigureScript();

    private:
        static ScriptManager* s_instance;

        ScriptManagerBase* m_base;

        struct ScriptItem {
            KURL url;
            KRun* process;
        };

        typedef QMap<QString, ScriptItem> ScriptMap;

        ScriptMap m_scripts;
};


#endif /* AMAROK_SCRIPTMANAGER_H */


