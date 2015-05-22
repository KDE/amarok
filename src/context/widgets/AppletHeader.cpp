/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#define DEBUG_PREFIX "AppletHeader"

#include "AppletHeader.h"

#include "TextScrollingWidget.h"
#include "core/support/Debug.h"

#include <Plasma/IconWidget>

#include <QApplication>
#include <QFont>
#include <QGraphicsLinearLayout>
#include <QPainter>
#include <QStyle>

Context::AppletHeader::AppletHeader( QGraphicsItem *parent, Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
    , m_mainLayout( new QGraphicsLinearLayout( Qt::Horizontal, this ) )
    , m_leftLayout( new QGraphicsLinearLayout( Qt::Horizontal ) )
    , m_rightLayout( new QGraphicsLinearLayout( Qt::Horizontal ) )
    , m_titleWidget( new TextScrollingWidget( this ) )
{
    QFont labelFont;
    labelFont.setPointSize( labelFont.pointSize() + 2 );
    m_titleWidget->setFont( labelFont );
    m_titleWidget->setDrawBackground( true );
    m_titleWidget->setText( i18n( "Context Applet" ) );
    m_titleWidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );

    m_mainLayout->setSpacing( 4 );
    m_mainLayout->addItem( m_leftLayout );
    m_mainLayout->addItem( m_titleWidget );
    m_mainLayout->addItem( m_rightLayout );
    m_mainLayout->setContentsMargins( 2, 4, 2, 2 );
    m_mainLayout->setStretchFactor( m_titleWidget, 10000 );
    m_mainLayout->setAlignment( m_leftLayout, Qt::AlignLeft );
    m_mainLayout->setAlignment( m_titleWidget, Qt::AlignHCenter );
    m_mainLayout->setAlignment( m_rightLayout, Qt::AlignRight );
    m_mainLayout->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    m_leftLayout->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );
    m_rightLayout->setSizePolicy( QSizePolicy::Maximum, QSizePolicy::Fixed );

    m_height = 4 + 2 + m_titleWidget->size().height()
        + QApplication::style()->pixelMetric( QStyle::PM_LayoutTopMargin )
        + QApplication::style()->pixelMetric( QStyle::PM_LayoutBottomMargin );
}

Context::AppletHeader::~AppletHeader()
{
}

qreal
Context::AppletHeader::height() const
{
    return m_height;
}

void
Context::AppletHeader::addIcon( Plasma::IconWidget *icon, Qt::Alignment align )
{
    if( !icon )
        return;

    clearDummyItems();
    if( align == Qt::AlignLeft )
        m_leftLayout->addItem( icon );
    else if( align == Qt::AlignRight )
        m_rightLayout->addItem( icon );
    else
        return;

    const int diff = m_leftLayout->count() - m_rightLayout->count();
    QGraphicsLinearLayout *layout = ( diff > 0 ) ? m_rightLayout : m_leftLayout;
    int index = ( diff > 0 ) ? 0 : -1;
    for( int i = 0, count = qAbs( diff ); i < count; ++i )
    {
        QGraphicsWidget *dummy = new QGraphicsWidget( this );
        dummy->setMinimumSize( icon->size() );
        dummy->setMaximumSize( icon->size() );
        dummy->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
        m_dummyItems << dummy;
        layout->insertItem( index, dummy );
    }
}

QString
Context::AppletHeader::titleText() const
{
    return m_titleWidget->text();
}

void
Context::AppletHeader::setTitleText( const QString &text )
{
    m_titleWidget->setScrollingText( text );
}

TextScrollingWidget *
Context::AppletHeader::textScrollingWidget()
{
    return m_titleWidget;
}

void
Context::AppletHeader::clearDummyItems()
{
    if( m_dummyItems.isEmpty() )
        return;

    QList<int> toRemove;
    for( int i = 0, count = m_leftLayout->count(); i < count; ++i )
    {
        QGraphicsLayoutItem *item = m_leftLayout->itemAt( i );
        if( m_dummyItems.contains( item ) )
        {
            m_dummyItems.removeAll( item );
            toRemove << i;
        }
    }
    while( !toRemove.isEmpty() )
    {
        int index = toRemove.takeLast();
        QGraphicsLayoutItem *item = m_leftLayout->itemAt( index );
        m_leftLayout->removeAt( index );
        delete item;
    }
    toRemove.clear();

    for( int i = 0, count = m_rightLayout->count(); i < count; ++i )
    {
        QGraphicsLayoutItem *item = m_rightLayout->itemAt( i );
        if( m_dummyItems.contains( item ) )
        {
            m_dummyItems.removeAll( item );
            toRemove << i;
        }
    }
    while( !toRemove.isEmpty() )
    {
        int index = toRemove.takeLast();
        QGraphicsLayoutItem *item = m_rightLayout->itemAt( index );
        m_rightLayout->removeAt( index );
        delete item;
    }
    m_dummyItems.clear();
}

