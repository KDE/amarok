/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef SCRIPTCONSOLE_H
#define SCRIPTCONSOLE_H

#include <QMainWindow>
#include <QDockWidget>
#include <QScriptEngineAgent>
#include <QtScriptTools/QScriptEngineDebugger>
#include <QWeakPointer>

namespace KTextEditor{
    class Editor;
    class View;
}
class QEvent;
class QListWidget;
class QListWidgetItem;
class QModelIndex;
class QScriptEngine;
class QSplitter;

namespace ScriptConsoleNS
{
class ScriptConsoleItem;
class ScriptListDockWidget;

    class ScriptConsole : public QMainWindow
    {
        Q_OBJECT

        public slots:
            static ScriptConsole *instance();
            void abortEvaluation();

        private slots:
            void slotExecuteNewScript();
            void slotNewScript();
            void setCurrentScriptItem( ScriptConsoleItem *item );
            void slotEvaluationSuspended();
            void slotEvaluationResumed();
            void slotEditScript( ScriptConsoleItem *item );

        private:
            explicit ScriptConsole( QWidget *parent );
            virtual ~ScriptConsole();

            bool eventFilter( QObject *watched, QEvent *event );
            QDockWidget *getWidget( const QString &title, QScriptEngineDebugger::DebuggerWidget widget );
            void closeEvent( QCloseEvent *event );
            ScriptConsoleItem* createScriptItem( const QString &script );

            QScriptEngineDebugger *m_debugger;
            QWeakPointer<ScriptConsoleItem> m_scriptItem;
            QDockWidget *m_codeWidget;
            QString m_savePath;
            KTextEditor::Editor *m_editor;
            ScriptListDockWidget *m_scriptListDock;
            static QWeakPointer<ScriptConsole> s_instance;
    };

    class ScriptListDockWidget : public QDockWidget
    {
        Q_OBJECT

        public:
            ScriptListDockWidget( QWidget *parent );
            ~ScriptListDockWidget();
            void addScript( ScriptConsoleItem *script );
            void addItem( QListWidgetItem *item );
            void prev();
            void next();

        public slots:
            void clear();
            void removeCurrentScript();

        signals:
            void edit( ScriptConsoleItem *item );
            void executeScript( ScriptConsoleItem *item );
            void currentItemChanged( ScriptConsoleItem *newItem );
            void newScript();

        private slots:
            void slotDoubleClicked( const QModelIndex &index );
            void slotCurrentItemChanged( QListWidgetItem *newItem, QListWidgetItem *oldItem );

        private:
            QListWidget *m_scriptListWidget;
            const int ScriptRole = 1002;
    };

    /*
    class DebuggerProxyAgent : public QScriptEngineAgent
    {
            DebuggerProxyAgent( QScriptEngine *engine );
            ~DebuggerProxyAgent();
            void contextPop();
            void contextPush();
            void exceptionCatch( qint64 scriptId, const QScriptValue &exception );
            void exceptionThrow( qint64 scriptId, const QScriptValue &exception, bool hasHandler );
            void functionEntry( qint64 scriptId );
            void functionExit( qint64 scriptId, const QScriptValue &returnValue );
            void positionChange( qint64 scriptId, int lineNumber, int columnNumber );
            void scriptLoad( qint64 id, const QString &program, const QString &fileName, int baseLineNumber );
            void scriptUnload( qint64 id );

        private:
            QScriptEngineAgent *d;
    };*/
}

#endif // SCRIPTCONSOLE_H
