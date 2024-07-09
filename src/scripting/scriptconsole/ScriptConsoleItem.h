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

#ifndef SCRIPT_CONSOLE_ITEM_H
#define SCRIPT_CONSOLE_ITEM_H

#include "scripting/scriptmanager/ScriptItem.h"

namespace KTextEditor{
    class View;
}
class QJSValue;
class QWidget;
class QPlainTextEdit;

namespace ScriptConsoleNS
{
    class ScriptEditorDocument;

    class ScriptConsoleItem : public ScriptItem
    {
        Q_OBJECT

        public:
            ScriptConsoleItem( QObject *parent, const QString &name, const QString &category
                            , const QString &path, ScriptEditorDocument *document );
            ~ScriptConsoleItem() override;
            ScriptEditorDocument* document() { return m_viewFactory; }
            bool start( bool silent = false ) override;
            KTextEditor::View *getEditorView( QWidget *parent );
            void appendToConsoleWidget( const QString &msg);
            QWidget *getConsoleWidget( QWidget *parent );
            QWidget *getOutputWdiget( QWidget *parent );
            QWidget *getErrorWidget( QWidget *parent );

            /**
             * Clear script files on disk upon object deletion
             */
            void setClearOnDeletion( bool clearOnDelete );
            void pause() override;

        public Q_SLOTS:
            void updateOutputWidget( QString output );
            void updateErrorWidget( QJSValue error );


        private:
            bool m_clearOnDelete;
            ScriptEditorDocument *m_viewFactory;
            QPointer<KTextEditor::View> m_view;
            QPointer<QPlainTextEdit> m_console;
            QPointer<QPlainTextEdit> m_output;
            QPointer<QPlainTextEdit> m_error;

            void timerEvent(QTimerEvent* event) override;
            void initializeScriptEngine() override;
            static KPluginMetaData createSpecFile( const QString &name, const QString &category, const QString &path );
            QString handleError( QJSValue *result ) override;

    };
}

Q_DECLARE_METATYPE( ScriptConsoleNS::ScriptConsoleItem* )

#endif // SCRIPT_CONSOLE_ITEM_H
