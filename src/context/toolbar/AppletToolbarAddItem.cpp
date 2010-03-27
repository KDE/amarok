/****************************************************************************************
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "AppletToolbarAddItem.h"

#include "Amarok.h"
#include "App.h"
#include "ContextView.h"
#include "core/support/Debug.h"
#include "PaletteHandler.h"

#include <KIcon>

#include <QAction>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QSizeF>
#include <QStyleOptionGraphicsItem>

#define MARGIN           10
#define TOOLBAR_X_OFFSET  5
#define TOOLBAR_Y_OFFSET  5

Context::AppletToolbarAddItem::AppletToolbarAddItem( QGraphicsItem* parent, Context::Containment* cont, bool fixedAdd )
    : AppletToolbarBase( parent )
    , m_iconPadding( 0 )
    , m_fixedAdd( fixedAdd )
    , m_showingAppletExplorer( false )
    , m_cont( cont )
    , m_icon( 0 )
    , m_label( 0 )
{
    QAction* listAdd = new QAction( i18n( "Add Applets..." ), this );
    listAdd->setIcon( KIcon( "list-add" ) );
    listAdd->setVisible( true );
    listAdd->setEnabled( true );
    
    connect( listAdd, SIGNAL( triggered() ), this, SLOT( iconClicked() ) );
    
    m_icon = new Plasma::IconWidget( this );

    m_icon->setAction( listAdd );
    m_icon->setText( QString() );
    m_icon->setToolTip( listAdd->text() );
    m_icon->setDrawBackground( false );
    m_icon->setOrientation( Qt::Horizontal );
    QSizeF iconSize;
    if( m_fixedAdd )
        iconSize = m_icon->sizeFromIconSize( 22 );
    else
        iconSize = m_icon->sizeFromIconSize( 11 );
    m_icon->setMinimumSize( iconSize );
    m_icon->setMaximumSize( iconSize );
    m_icon->resize( iconSize );
    m_icon->setZValue( zValue() + 1 );
    
    m_label = new QGraphicsSimpleTextItem( i18n( "Add Applet..." ), this );
    m_label->hide();
    
    if( m_cont )
        connect( m_cont->view(), SIGNAL( appletExplorerHid() ), this, SLOT( appletExplorerHid() ) );

    if( m_fixedAdd )
        setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    else
        setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Preferred );
  //  resize( QSizeF( 18, 24 ) );
}

Context::AppletToolbarAddItem::~AppletToolbarAddItem()
{}

void
Context::AppletToolbarAddItem::appletExplorerHid() // SLOT
{
    m_showingAppletExplorer = false;
}
void 
Context::AppletToolbarAddItem::iconClicked() // SLOT
{
    if( m_showingAppletExplorer )
    {
        m_showingAppletExplorer = false;
        emit hideAppletExplorer();
    }
    else
    {
        m_showingAppletExplorer = true;
        emit showAppletExplorer();
    }
}

void 
Context::AppletToolbarAddItem::resizeEvent( QGraphicsSceneResizeEvent * event )
{
    Q_UNUSED( event )
    if( m_label->boundingRect().width() < ( boundingRect().width() - 2*m_icon->boundingRect().width() ) )
        // do we have size to show it?
    {
        m_icon->setPos(  boundingRect().width() - m_icon->boundingRect().width(), ( boundingRect().height() / 2 ) - (
m_icon->size().height() / 2 ) );
        m_label->setPos( ( boundingRect().width() / 2 ) - ( m_label->boundingRect().width() / 2 ),  (
boundingRect().height() / 2 ) - ( m_label->boundingRect().height() / 2 ) );
        m_label->show();
    } else 
    {        
        m_icon->setPos( ( boundingRect().width() / 2 ) - ( m_icon->boundingRect().width() / 2 ) , (
boundingRect().height() / 2 ) - ( m_icon->size().height() / 2 ) );
        m_label->hide();
    }
}


QSizeF
Context::AppletToolbarAddItem::sizeHint( Qt::SizeHint which, const QSizeF & constraint ) const
{
    if( m_fixedAdd )
    //    return QSizeF( m_icon->size().width() + 2 * m_iconPadding, QGraphicsWidget::sizeHint( which,constraint
//).height() );
        return QGraphicsWidget::sizeHint(which, constraint);
    else
        if( which == Qt::MinimumSize )
            return QSizeF();
        else
            return QSizeF( m_icon->size().width() + 2 * m_iconPadding, QGraphicsWidget::sizeHint( which, constraint
).height() );
    
}

void 
Context::AppletToolbarAddItem::mousePressEvent( QGraphicsSceneMouseEvent * event )
{
    DEBUG_BLOCK
    emit showAppletExplorer();
    event->accept();
}

#include "AppletToolbarAddItem.moc"
