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

#ifndef SCRIPT_EDITOR_DOCUMENT_H
#define SCRIPT_EDITOR_DOCUMENT_H

#include <QObject>

namespace KTextEditor
{
    class Document;
    class View;
}
class KUrl;
class QIcon;
class QWidget;

namespace ScriptConsole
{
    class ScriptEditorDocument : public QObject
    {
        public:
            ScriptEditorDocument( KTextEditor::Document* document );
            virtual ~ScriptEditorDocument();
            QString text() const;
            KTextEditor::View *createView( QWidget *editor = 0 );
            void setText( const QString &text );
            void save( const KUrl &url );
            void save();

        private:
            KTextEditor::Document *m_document;
    };
}

#endif // SCRIPT_EDITOR_DOCUMENT_H