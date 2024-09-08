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

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <KTextEditor/CodeCompletionInterface>
#include <KTextEditor/ConfigInterface>
#include <KTextEditor/MovingInterface>
#endif

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
    //TODO completion Qt6
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    KTextEditor::CodeCompletionInterface *codeCompletionIf = qobject_cast<KTextEditor::CodeCompletionInterface*>( view );
    if( codeCompletionIf )
    {
        if( !s_completionModel )
            s_completionModel = new AmarokScriptCodeCompletionModel( parent );
        codeCompletionIf->registerCompletionModel( s_completionModel.data() );
        codeCompletionIf->setAutomaticInvocationEnabled( true );
    }
    KTextEditor::ConfigInterface *configIface = qobject_cast<KTextEditor::ConfigInterface*>( view );
    if( configIface )
        configIface->setConfigValue( QStringLiteral("line-numbers"), true );
#endif
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

void
ScriptEditorDocument::highlight( KTextEditor::View *view, int line, const QColor &color )
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    KTextEditor::MovingInterface *movingIf = qobject_cast<KTextEditor::MovingInterface*>( view->document() );
    if( !movingIf )
      return;

    KTextEditor::MovingRange *movingRange = movingIf->newMovingRange( KTextEditor::Range( line, 0, line, 500 ) );
    movingRange->setView( view );
    movingRange->setZDepth( -999 );
    //use highlightinterface::default styles?
    KTextEditor::Attribute::Ptr attrb( new KTextEditor::Attribute() );
    attrb->setBackground( color );
    movingRange->setAttribute( attrb );
#else
    Q_UNUSED(view) //TODO highlight Qt6
    Q_UNUSED(line)
    Q_UNUSED(color)
#endif
}
