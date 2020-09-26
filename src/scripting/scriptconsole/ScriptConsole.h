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
#include <QPointer>

namespace KTextEditor{
    class Editor;
}
class QEvent;
class QListWidget;
class QListWidgetItem;
class QModelIndex;
class QJSEngine;

namespace ScriptConsoleNS
{
class ScriptConsoleItem;
class ScriptListDockWidget;

    class ScriptConsole : public QMainWindow
    {
        Q_OBJECT

        public:
            static ScriptConsole *instance();

        private Q_SLOTS:
            void slotAbortEvaluation();
            void slotExecuteNewScript();
            void slotNewScript();
            void setCurrentScriptItem( ScriptConsoleItem *item );
            void slotEvaluationSuspended();
            void slotEvaluationResumed();
            void slotEditScript( ScriptConsoleItem *item );

        private:
            explicit ScriptConsole( QWidget *parent );
            ~ScriptConsole() override;

            bool eventFilter( QObject *watched, QEvent *event ) override;
            QDockWidget *getWidget( const QString &title, QWidget *widget );
            void closeEvent( QCloseEvent *event ) override;
            ScriptConsoleItem* createScriptItem( const QString &script );
            ScriptListDockWidget *getScriptListDockWidget();

            QPointer<ScriptConsoleItem> m_scriptItem;
            QDockWidget *m_consoleWidget;
            QDockWidget *m_codeWidget;
            QDockWidget *m_outputWidget;
            QDockWidget *m_errorWidget;
            QString m_savePath;
            KTextEditor::Editor *m_editor;
            ScriptListDockWidget *m_scriptListDock;
            static QPointer<ScriptConsole> s_instance;
    };

    class ScriptListDockWidget : public QDockWidget
    {
        Q_OBJECT

        public:
            explicit ScriptListDockWidget( QWidget *parent );
            ~ScriptListDockWidget() override;
            QListWidget *listWidget();
            void addScript( ScriptConsoleItem *script );
            ScriptConsoleItem *getScript( const QString &scriptName);
            void addItem( QListWidgetItem *item );

        public Q_SLOTS:
            void clear();
            void removeCurrentScript();
            void prev();
            void next();

        Q_SIGNALS:
            void edit( ScriptConsoleItem *item );
            void executeScript( ScriptConsoleItem *item );
            void currentItemChanged( ScriptConsoleItem *newItem );
            void newScript();

        private Q_SLOTS:
            void slotDoubleClicked( const QModelIndex &index );
            void slotCurrentItemChanged( QListWidgetItem *newItem, QListWidgetItem *oldItem );

        private:
            QListWidget *m_scriptListWidget;
            const int ScriptRole = 1002;
    };
}

#endif // SCRIPTCONSOLE_H
