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

#include "CompletionModel.h"
#include "ScriptConsole.h"

#include <KTextEditor/Attribute>
#include <KTextEditor/Document>
#include <KTextEditor/MovingRange>
#include <KTextEditor/View>

using namespace ScriptConsoleNS;

QPointer<AmarokScriptCodeCompletionModel> ScriptEditorDocument::s_completionModel;

ScriptEditorDocument::ScriptEditorDocument( QObject *parent, KTextEditor::Document* document )
: QObject( parent )
{
    m_document = document;
    m_document->setParent( this );
    m_document->setHighlightingMode(QStringLiteral("JavaScript"));
}

KTextEditor::View*
ScriptEditorDocument::createView( QWidget* parent )
{
    KTextEditor::View *view = m_document->createView( parent );
    if( !s_completionModel )
        s_completionModel = new AmarokScriptCodeCompletionModel( parent );
    view->registerCompletionModel( s_completionModel.data() );
    view->setAutomaticInvocationEnabled( true );

    view->setConfigValue( QStringLiteral("line-numbers"), true );

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
ScriptEditorDocument::save( const QUrl &url )
{
    m_document->saveAs( url );
}

ScriptEditorDocument::~ScriptEditorDocument()
{}

void
ScriptEditorDocument::save()
{
    m_document->save();
}

void
ScriptEditorDocument::setReadWrite( bool readWrite )
{
    m_document->setReadWrite( readWrite );
}

