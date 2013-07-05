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

#include "ui_ScriptConsole.h"

#include <KDialog>

#include <QWeakPointer>

namespace KTextEditor{
    class Editor;
    class View;
}
class QEvent;
class QListWidget;
class QModelIndex;
class QScriptEngine;
class QSplitter;

namespace ScriptConsole
{
    class ScriptConsoleItem;

    class ScriptConsoleDialog : public KDialog, private Ui_ScriptConsole
    {
        Q_OBJECT

        public slots:
            static ScriptConsoleDialog *instance();

        private slots:
            void slotExecuteNewScript();
            void slotKillAllAndClear();
            void slotToggleScript( const QModelIndex &index );
            void slotKillAndClearScript( const QModelIndex &index );
            void slotBackHistory();
            void slotForwardHistory();
            void dataChanged();

        private:
            explicit ScriptConsoleDialog( QWidget *parent );
            virtual ~ScriptConsoleDialog();
            void closeEvent( QCloseEvent *event );
            void keyPressEvent( QKeyEvent *event );
            ScriptConsoleItem* addItem( const QString &script );

            KTextEditor::Editor *m_editor;
            KTextEditor::View *m_view;
            QString m_savePath;
            static QWeakPointer<ScriptConsoleDialog> s_instance;
            int m_currentItemIndex;
    };
}

#endif // SCRIPTCONSOLE_H
