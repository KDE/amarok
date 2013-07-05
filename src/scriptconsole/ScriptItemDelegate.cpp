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

#include "ScriptItemDelegate.h"

#include "App.h"
#include "ScriptConsoleItem.h"
#include "ScriptEditorDocument.h"

#include <KTextEditor/Document>
#include <KTextEditor/View>

#include <QList>
#include <QMouseEvent>
#include <KDialog>
#include <QPainter>
#include <QScriptEngine>

using namespace ScriptConsole;

ScriptItemDelegate::ScriptItemDelegate( QObject* parent )
: QStyledItemDelegate( parent )
{
    if( m_killAndClearIcon.isNull() )
    {
        m_killAndClearIcon = App::instance()->style()->standardPixmap( QStyle::SP_DialogCloseButton );
        m_scriptStartIcon = App::instance()->style()->standardPixmap( QStyle::SP_MediaPlay );
        m_scriptStopIcon = App::instance()->style()->standardPixmap( QStyle::SP_MediaStop );
    }
}

QPoint
ScriptItemDelegate::toggleRunIconPosition( const QStyleOptionViewItem &option ) const
{
    return QPoint( killAndClearIconPosition( option ).x() - m_killAndClearIcon.width()/2 - m_scriptStartIcon.width()/2 - margin*15,
                   option.rect.center().y() - m_scriptStartIcon.height()/2 );
}

QPoint
ScriptItemDelegate::killAndClearIconPosition( const QStyleOptionViewItem &option ) const
{
    return QPoint( option.rect.right() - m_killAndClearIcon.width() - margin,
                   option.rect.center().y() - m_killAndClearIcon.height()/2 );
}

void
ScriptItemDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    ScriptConsoleItem *item = qvariant_cast<ScriptConsoleItem*>( index.data( Script ) );

    if( !item )
    {
        QStyledItemDelegate::paint( painter, option, index );
        return;
    }

    if (option.state & QStyle::State_Selected)
        painter->fillRect(option.rect, option.palette.highlight());

    if ( option.state & QStyle::State_MouseOver )
    {
        painter->drawPixmap( killAndClearIconPosition( option ), m_killAndClearIcon );
        if( item->engine() && item->engine()->isEvaluating() )
            painter->drawPixmap( toggleRunIconPosition( option ), m_scriptStopIcon );
        else
            painter->drawPixmap( toggleRunIconPosition( option ), m_scriptStartIcon  );
    }
    QRect r = option.rect;

    m_imageSpace = 10 + option.rect.width()/25;

    /*QIcon ic = QIcon( qvariant_cast<QPixmap>( index.data( Qt::DecorationRole ) ).scaled(
     *            QSize( m_imageSpace, m_imageSpace ), Qt::KeepAspectRatio
    ) );*/

    QIcon ic;// = item->info().icon().pixmap( m_imageSpace, m_imageSpace );
    QString title = item->name();

    if (!ic.isNull())
    {
        r = option.rect.adjusted(5, 10, -10, -10);
        ic.paint( painter, r, Qt::AlignVCenter | Qt::AlignLeft );
    }

    r = option.rect.adjusted( m_imageSpace + margin*5, 0, -10, -30 );
    //painter->setFont( QFont( "Lucida Grande", 15, QFont::Normal ) );
    painter->drawText( r.left(), r.top(), r.width(), r.height(), Qt::AlignTop | Qt::AlignLeft, title, &r );

    r = option.rect.adjusted( m_imageSpace + margin*5, 30, -10, 0);
    //painter->setFont( QFont( "Lucida Grande", 10, QFont::Normal ) );
    painter->drawText( r.left(), r.top(), r.width(), r.height(), Qt::AlignTop|Qt::AlignLeft|Qt::TextWordWrap
                        , QString( "%1\n%2\n\n%3\n%4\n\n%5\n%6" )
                        .arg( i18n("Code") )
                        .arg( item->document()->text() )
                        .arg( i18n("Output") )
                        .arg( item->output() )
                        .arg( i18n("Error Log") )
                        .arg( item->log().join("\n") ), &r );
}

QSize
ScriptItemDelegate::sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    ScriptConsoleItem *item = qvariant_cast<ScriptConsoleItem*>( index.data( Script ) );
    if( !item )
        return QStyledItemDelegate::sizeHint( option, index );
    QSize s1 = option.fontMetrics.boundingRect( option.rect.adjusted( m_imageSpace, 0, -toggleRunIconPosition( option ).x() - m_scriptStartIcon.width()/2, 0 ), Qt::TextWordWrap,
                                                QString( "%1\n%2\n\n%3\n%4\n\n%5\n%6" )
                                                .arg( i18n("Code") )
                                                .arg( item->document()->text() )
                                                .arg( i18n("Output") )
                                                .arg( item->output() )
                                                .arg( i18n("Error Log") )
                                                .arg( item->log().join("\n") ) ).size();
    QSize sret;
    sret.rwidth() = s1.width()+ m_killAndClearIcon.width() + m_scriptStartIcon.width() + margin*2;// + size2.width();;
    sret.rheight() = s1.height() + m_scriptStartIcon.height() + margin * 2 + 20;
    return sret;
}

bool
ScriptItemDelegate::editorEvent( QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option,
                                 const QModelIndex& index)
{
    if( event->type() == QEvent::MouseButtonRelease )
    {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if( m_killAndClearIcon.rect().translated( killAndClearIconPosition( option ) ).contains( mouseEvent->pos() ) )
            emit killAndClearButtonClicked( index );
        else if( m_scriptStartIcon.rect().translated( toggleRunIconPosition( option ) ).contains( mouseEvent->pos() ) )
            emit toggleRunButtonClicked( index );
    }
    return QStyledItemDelegate::editorEvent(event, model, option, index);
}

QWidget*
ScriptItemDelegate::createEditor( QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    ScriptConsoleItem *item = qvariant_cast<ScriptConsoleItem*>( index.data(Script) );
    KTextEditor::View *editor = item->createEditorView( parent );
    if( editor )
    {
        connect( editor, SIGNAL(focusOut(KTextEditor::View*)), this, SLOT(commitAndCloseEditor()) );
        return editor;
    }
    else
        return 0;
}

void
ScriptItemDelegate::commitAndCloseEditor()
{
    KTextEditor::View *editor = qobject_cast<KTextEditor::View*>( sender() );
    if( editor )
        editor->document()->save();
    emit commitData( editor );
    emit closeEditor( editor );
}

void
ScriptItemDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
    ScriptConsoleItem *item = qvariant_cast<ScriptConsoleItem*>( index.data( Script ) );
    if( item )
        model->setData( index, qVariantFromValue<ScriptConsoleItem*>( item ), Script );
    else
        QStyledItemDelegate::setModelData( editor, model, index );
}
