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

#include "CompletionModel.h"

#include "core/support/Debug.h"

#include <QFile>
#include <QStandardPaths>

#include <KTextEditor/View>
#include <KTextEditor/Document>

using namespace ScriptConsoleNS;

AmarokScriptCodeCompletionModel::AmarokScriptCodeCompletionModel( QObject *parent )
    : CodeCompletionModel( parent )
{
    const QUrl url( QStandardPaths::locate( QStandardPaths::GenericDataLocation, QStringLiteral("amarok/scriptconsole/") ) );
    QFile file( url.path() + QStringLiteral("AutoComplete.txt") );
    if( file.open( QFile::ReadOnly ) )
    {
        QTextStream in( &file );
        while ( !in.atEnd() )
            m_autoCompleteStrings << in.readLine();
    }
    else
        debug() << "No autocomplete file found for the script console";
}

void
AmarokScriptCodeCompletionModel::completionInvoked( KTextEditor::View *view, const KTextEditor::Range &range, KTextEditor::CodeCompletionModel::InvocationType invocationType )
{
    Q_UNUSED( invocationType )

    beginResetModel();
    m_completionList.clear();
    const QString &currentText = view->document()->text( range );
    for( const QString &completionItem : m_autoCompleteStrings )
    {
        int index = completionItem.indexOf( currentText, Qt::CaseInsensitive ) + currentText.length();
        if( index != -1 && !completionItem.mid(index, completionItem.size()-index ).contains( QLatin1Char('.') ) && completionItem != currentText )
            m_completionList << completionItem;
    }
    setRowCount( m_completionList.count() );
    endResetModel();
}

QVariant
AmarokScriptCodeCompletionModel::data( const QModelIndex &index, int role ) const
{
    if( !index.isValid() || role != Qt::DisplayRole || index.row() < 0 || index.row() >= rowCount()
        || index.column() != KTextEditor::CodeCompletionModel::Name )
        return QVariant();
    return m_completionList[ index.row() ];
}

KTextEditor::Range
AmarokScriptCodeCompletionModel::completionRange(KTextEditor::View* view, const KTextEditor::Cursor& position)
{
    const QString& line = view->document()->line(position.line());
    KTextEditor::Range range(position, position);
    // include everything non-space before
    for( int i = position.column() - 1; i >= 0; --i )
    {
        if( line.at( i ).isSpace() )
            break;
        else
            range.start().setColumn( i );
    }
    // include everything non-space after
    for( int i = position.column() + 1; i < line.length(); ++i )
    {
        if( line.at( i ).isSpace() )
            break;
        else
            range.end().setColumn( i );
    }
    return range;
}

bool
AmarokScriptCodeCompletionModel::shouldAbortCompletion( KTextEditor::View *view, const KTextEditor::Range &range, const QString &currentCompletion )
{
    if(view->cursorPosition() < range.start() || view->cursorPosition() > range.end())
        return true; //Always abort when the completion-range has been left

    for( int i = 0; i < currentCompletion.length(); ++i )
    {
        if( currentCompletion.at( i ).isSpace() )
            return true;
    }
    // else it's valid
    return false;
}

void
AmarokScriptCodeCompletionModel::executeCompletionItem( KTextEditor::View *view, const KTextEditor::Range &range, const QModelIndex &index ) const
{
    view->document()->replaceText( range, m_completionList.at( index.row() ) );
}

AmarokScriptCodeCompletionModel::~AmarokScriptCodeCompletionModel()
{
    DEBUG_BLOCK
    m_completionList.clear();
}
