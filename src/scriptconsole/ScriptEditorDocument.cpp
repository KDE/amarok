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

#include "ScriptEditorDocument.h"

#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/Document>
#include <KTextEditor/View>

using namespace ScriptConsole;

ScriptEditorDocument::ScriptEditorDocument( KTextEditor::Document* document )
{
    m_document = document;
    m_document->setParent( this );
}

KTextEditor::View*
ScriptEditorDocument::createView( QWidget* parent )
{
    KTextEditor::View *view = qobject_cast<KTextEditor::View*>( m_document->createView( parent ) );
    KTextEditor::CodeCompletionInterface *iface = qobject_cast<KTextEditor::CodeCompletionInterface*>( view );
    if( iface )
    {
        //iface->registerCompletionModel();
    }
    // enable the modified on disk warning dialogs if any
    //if (qobject_cast<KTextEditor::ModificationInterface*>(doc))
    //qobject_cast<KTextEditor::ModificationInterface*>(doc)->setModifiedOnDiskWarning (true);
    return view;
}

QString
ScriptEditorDocument::text() const
{
    return m_document->text();
}

void
ScriptEditorDocument::setText( const QString &text )
{
    m_document->setText( text );
}

void
ScriptEditorDocument::save( const KUrl &url )
{
    m_document->saveAs( url );
}

ScriptEditorDocument::~ScriptEditorDocument()
{}

void ScriptEditorDocument::save()
{
    m_document->save();
}
