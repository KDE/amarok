/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "PrettyTreeView.h"

#include "PaletteHandler.h"
#include "SvgHandler.h"
#include "widgets/PrettyTreeRoles.h"
#include "widgets/PrettyTreeDelegate.h"

#include <QAction>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

Q_DECLARE_METATYPE( QAction* )
Q_DECLARE_METATYPE( QList<QAction*> )

using namespace Amarok;

PrettyTreeView::PrettyTreeView( QWidget *parent )
    : QTreeView( parent )
    , m_decoratorActionPressed( nullptr )
{
    setAlternatingRowColors( true );
    setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );

    The::paletteHandler()->updateItemView( this );
    connect( The::paletteHandler(), &PaletteHandler::newPalette, this, &PrettyTreeView::newPalette );

#ifdef Q_OS_APPLE
    // for some bizarre reason w/ some styles on mac per-pixel scrolling is slower than
    // per-item
    setVerticalScrollMode( QAbstractItemView::ScrollPerItem );
    setHorizontalScrollMode( QAbstractItemView::ScrollPerItem );
#else
    // Scrolling per item is really not smooth and looks terrible
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel );
#endif

    setAnimated( true );
}

PrettyTreeView::~PrettyTreeView()
{
}

void
PrettyTreeView::edit( const QModelIndex &index )
{
    QTreeView::edit( index );
}

bool
PrettyTreeView::edit( const QModelIndex &index, QAbstractItemView::EditTrigger trigger, QEvent *event )
{
    QModelIndex parent = index.parent();
    while( parent.isValid() )
    {
        expand( parent );
        parent = parent.parent();
    }
    return QAbstractItemView::edit( index, trigger, event );
}

void
PrettyTreeView::drawRow( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
    QTreeView::drawRow( painter, option, index );

    const int width = option.rect.width();
    const int height = option.rect.height();

    if( height > 0 )
    {
        QPixmap background = The::svgHandler()->renderSvgWithDividers(
                QStringLiteral("service_list_item"), width, height, QStringLiteral("service_list_item") );

        painter->save();
        painter->drawPixmap( option.rect.topLeft().x(), option.rect.topLeft().y(), background );
        painter->restore();
    }
}

void
PrettyTreeView::mouseMoveEvent( QMouseEvent *event )
{
    // swallow the mouse move event in case the press was started on decorator action icon
    if( m_decoratorActionPressed )
        event->accept();
    else
        QTreeView::mouseMoveEvent( event );

    // Make sure we repaint the item for the collection action buttons
    const QModelIndex index = indexAt( event->pos() );
    const int actionsCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();
    if( actionsCount )
        update( index );
}

void
PrettyTreeView::mousePressEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );

    // reset state variables on every mouse button press
    m_expandCollapsePressedAt.reset();
    m_decoratorActionPressed = nullptr;

    // if root is decorated, it doesn't show any actions
    QAction *action = rootIsDecorated() ? nullptr : decoratorActionAt( index, event->pos() );
    if( action &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        state() == QTreeView::NoState )
    {
        m_decoratorActionPressed = action;
        update( index ); // trigger repaint to change icon effect
        event->accept();
        return;
    }

    bool prevExpandState = isExpanded( index );

    // This will toggle the expansion of the current item when clicking
    // on the fold marker but not on the item itself. Required here to
    // enable dragging.
    QTreeView::mousePressEvent( event );

    // if we press left mouse button on valid item which did not cause the expansion,
    // set m_expandCollapsePressedAt so that mouseReleaseEvent can perform the
    // expansion/collapsing
    if( index.isValid() &&
        prevExpandState == isExpanded( index ) &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        state() == QTreeView::NoState )
    {
        m_expandCollapsePressedAt.reset( new QPoint( event->pos() ) );
    }
}

void
PrettyTreeView::mouseReleaseEvent( QMouseEvent *event )
{
    const QModelIndex index = indexAt( event->pos() );
    // we want to reset m_expandCollapsePressedAt in either case, but still need its value
    QScopedPointer<QPoint> expandCollapsePressedAt( m_expandCollapsePressedAt.take() );
    // ditto for m_decoratorActionPressed
    QAction *decoratorActionPressed = m_decoratorActionPressed;
    m_decoratorActionPressed = nullptr;

    // if root is decorated, it doesn't show any actions
    QAction *action = rootIsDecorated() ? nullptr : decoratorActionAt( index, event->pos() );
    if( action &&
        action == decoratorActionPressed &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier )
    {
        action->trigger();
        update( index ); // trigger repaint to change icon effect
        event->accept();
        return;
    }

    if( index.isValid() &&
        event->button() == Qt::LeftButton &&
        event->modifiers() == Qt::NoModifier &&
        state() == QTreeView::NoState &&
        expandCollapsePressedAt &&
        ( *expandCollapsePressedAt - event->pos() ).manhattanLength() < QApplication::startDragDistance() &&
        style()->styleHint( QStyle::SH_ItemView_ActivateItemOnSingleClick, nullptr, this ) &&
        model()->hasChildren( index ) )
    {
        setExpanded( index, !isExpanded( index ) );
        event->accept();
        return;
    }

    QTreeView::mouseReleaseEvent( event );
}

bool
PrettyTreeView::viewportEvent( QEvent *event )
{
    if( event->type() == QEvent::ToolTip )
    {
        QHelpEvent *helpEvent = static_cast<QHelpEvent *>( event );
        const QModelIndex index = indexAt( helpEvent->pos() );
        // if root is decorated, it doesn't show any actions
        QAction *action = rootIsDecorated() ? nullptr : decoratorActionAt( index, helpEvent->pos() );
        if( action )
        {
            QToolTip::showText( helpEvent->globalPos(), action->toolTip() );
            event->accept();
            return true;
        }
    }

    // swallow the mouse hover event in case the press was started on decorator action icon
    // friend mouse move event is handled in mouseMoveEvent and triggers repaints
    if( event->type() == QEvent::HoverMove && m_decoratorActionPressed )
    {
        event->accept();
        return true;
    }

    return QAbstractItemView::viewportEvent( event );
}

QAction *
PrettyTreeView::decoratorActionAt( const QModelIndex &index, const QPoint &pos )
{
    const int actionsCount = index.data( PrettyTreeRoles::DecoratorRoleCount ).toInt();
    if( actionsCount <= 0 )
        return nullptr;

    PrettyTreeDelegate* ptd = qobject_cast<PrettyTreeDelegate*>( itemDelegate( index ) );
    if( !ptd )
        return nullptr;

    const QList<QAction *> actions = index.data( PrettyTreeRoles::DecoratorRole ).value<QList<QAction *> >();
    QRect rect = visualRect( index );

    for( int i = 0; i < actions.count(); i++ )
        if( ptd->decoratorRect( rect, i ).contains( pos ) )
            return actions.at( i );

    return nullptr;
}

QAction *
PrettyTreeView::pressedDecoratorAction() const
{
    return m_decoratorActionPressed;
}

void
PrettyTreeView::newPalette( const QPalette & palette )
{
    Q_UNUSED( palette )
    The::paletteHandler()->updateItemView( this );
    reset(); // redraw all potential delegates
}
