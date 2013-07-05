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

#ifndef SCRIPT_ITEM_DELEGATE_H
#define SCRIPT_ITEM_DELEGATE_H

#include <QStyledItemDelegate>

class QScriptEngine;
class QPainter;
class QTemporaryFile;

namespace ScriptConsole
{
    class ScriptItemDelegate : public QStyledItemDelegate
    {
        Q_OBJECT

    public:
        enum
        {
            Script = 1002
        };

        ScriptItemDelegate( QObject *parent );
        QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
        void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
        bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index);

    private:
        QPoint killAndClearIconPosition(const QStyleOptionViewItem &option) const;
        QPoint toggleRunIconPosition(const QStyleOptionViewItem &option) const;
        QWidget *createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index ) const;
        void setModelData( QWidget *editor, QAbstractItemModel *model, const QModelIndex &index ) const;

        QPixmap m_killAndClearIcon;
        QPixmap m_scriptStartIcon;
        QPixmap m_scriptStopIcon;
        static const int margin = 2;
        mutable int m_imageSpace;

    signals:
        void toggleRunButtonClicked(QModelIndex);
        void killAndClearButtonClicked(QModelIndex);

    private slots:
        void commitAndCloseEditor();
    };
}

#endif // SCRIPT_ITEM_DELEGATE_H