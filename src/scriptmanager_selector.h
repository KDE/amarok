// (c) 2003 Scott Wheeler <wheeler@kde.org>,
// (c) 2004 Mark Kretschmann <markey@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_SCRIPTMANAGER_SELECTOR_H
#define AMAROK_SCRIPTMANAGER_SELECTOR_H

#include <kdialogbase.h>    //baseclass

class ScriptManagerBase;


namespace ScriptManager
{
    class Selector : public KDialogBase
    {
            Q_OBJECT

        public:
            struct Result
            {
                QStringList dirs;
                QStringList addedDirs;
                QStringList removedDirs;
                DialogCode status;
            };

            Selector( const QStringList& directories, QWidget *parent = 0, const char *name = 0 );
            virtual ~Selector();

            static Selector* instance;
            
        public slots:
            Result exec();

        signals:
            void signalDirectoryAdded( const QString &directory );
            void signalDirectoryRemoved( const QString &directory );
            void signalRunScript( const QString &script );
            void signalStopScript( const QString &script );
            void signalConfigureScript( const QString &script );
            
        private slots:
            void slotAddDirectory();
            void slotRemoveDirectory();
            void slotRunScript();
            void slotStopScript();
            void slotConfigureScript();

        private:
            QStringList m_dirList;
            ScriptManagerBase *m_base;
            Result m_result;
    };
}


#endif /* AMAROK_SCRIPTMANAGER_SELECTOR_H */


