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

#ifndef SCRIPTCONSOLE_COMPLETIONMODEL_H
#define SCRIPTCONSOLE_COMPLETIONMODEL_H

#include <KTextEditor/CodeCompletionModel>
#include <KTextEditor/CodeCompletionModelControllerInterface>

namespace KTextEditor {
    class View;
}

namespace ScriptConsoleNS
{
    class AmarokScriptCodeCompletionModel : public KTextEditor::CodeCompletionModelControllerInterface
                                          , public KTextEditor::CodeCompletionModel
    {
        public:
            explicit AmarokScriptCodeCompletionModel( QObject *parent );
            ~AmarokScriptCodeCompletionModel() override;

        private:
            QVariant data( const QModelIndex &index, int role = Qt::DisplayRole ) const override;
            void completionInvoked( KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType ) override;
            void executeCompletionItem( KTextEditor::View *view, const KTextEditor::Range &range, const QModelIndex &index ) const override;
            KTextEditor::Range completionRange( KTextEditor::View *view, const KTextEditor::Cursor &position ) override;
            bool shouldAbortCompletion( KTextEditor::View *view, const KTextEditor::Range &range, const QString &currentCompletion ) override;

            QStringList m_completionList;
            QStringList m_autoCompleteStrings;
    };
}

#endif // SCRIPTCONSOLE_COMPLETIONMODEL_H
