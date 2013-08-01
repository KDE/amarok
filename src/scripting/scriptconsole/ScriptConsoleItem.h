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
class KUrl;
class QIcon;
class QScriptContext;
class QScriptEngine;
class QWidget;

namespace ScriptConsole
{
    class ScriptEditorDocument;

    class ScriptingMethods : public QObject
    {
        Q_OBJECT

        public:
            ScriptingMethods( QObject *parent );

        public slots:
            void print( const QScriptValue &value );

        signals:
            void output( QString );
    };

    class ScriptConsoleItem : public ScriptItem
    {
        Q_OBJECT

        public:
            ScriptConsoleItem( QObject *parent, const QString &name, const QString &path
                            , const QString &category, ScriptEditorDocument *document );
            virtual ~ScriptConsoleItem();
            ScriptEditorDocument* document() { return m_viewFactory; }
            QStringList log() { return m_log; }
            bool start( bool silent = false );
            KTextEditor::View *createEditorView( QWidget *parent );

            /**
             * Clear script files on disk upon object deletion
             */
            void setClearOnDeletion( bool clearOnDelete );

        private:
            ScriptEditorDocument *m_viewFactory;
            bool m_clearOnDelete;
            ScriptingMethods *m_scriptMethods;

            void initializeScriptEngine();
            static KPluginInfo createSpecFile( const QString &name, const QString &category, const QString &path );

        signals:
            void logChanged();

        private slots:
            void slotOutput( const QString &string );
    };
}
Q_DECLARE_METATYPE( ScriptConsole::ScriptConsoleItem* )

#endif // SCRIPT_CONSOLE_ITEM_H
