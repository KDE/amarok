/****************************************************************************************
 * Copyright (c) 2010 Daniel Faust <hessijames@gmail.com>                               *
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

#include "LabelGraphicsItem.h"
#include "LabelOverlayButton.h"

// Amarok
#include "PaletteHandler.h"

// KDE
#include <KIconLoader>

// Qt
#include <QApplication>
#include <QGraphicsSceneHoverEvent>
#include <QPropertyAnimation>


LabelGraphicsItem::LabelGraphicsItem( const QString &text, qreal deltaPointSize, QGraphicsItem *parent )
    : QGraphicsTextItem( text, parent ),
    m_selected( false )
{
    setAcceptHoverEvents( true );

    m_hoverColor = PaletteHandler::highlightColor( 0.7, 1.0 );

    m_hoverValueAnimation = new QPropertyAnimation( this, "hoverValue", this );
    m_hoverValueAnimation.data()->setEasingCurve( QEasingCurve::InOutQuad );

    KIconLoader *iconLoader = new KIconLoader();
    m_addLabelItem = new LabelOverlayButton( this );
    m_addLabelItem.data()->setPixmap( iconLoader->loadIcon( "list-add", KIconLoader::NoGroup, KIconLoader::SizeSmallMedium ) );
    m_addLabelItem.data()->setToolTip( i18n( "Add label" ) );
    m_addLabelItem.data()->setOpacity( 0.0 );
    m_removeLabelItem = new LabelOverlayButton( this );
    m_removeLabelItem.data()->setPixmap( iconLoader->loadIcon( "list-remove", KIconLoader::NoGroup, KIconLoader::SizeSmallMedium ) );
    m_removeLabelItem.data()->setToolTip( i18n( "Remove label" ) );
    m_removeLabelItem.data()->setOpacity( 0.0 );
    m_listLabelItem = new LabelOverlayButton( this );
    m_listLabelItem.data()->setPixmap( iconLoader->loadIcon( "edit-find", KIconLoader::NoGroup, KIconLoader::SizeSmallMedium ) );
    m_listLabelItem.data()->setToolTip( i18n( "Show in Media Sources" ) );
    m_listLabelItem.data()->setOpacity( 0.0 );
    m_blacklistLabelItem = new LabelOverlayButton( this );
    m_blacklistLabelItem.data()->setPixmap( iconLoader->loadIcon( "flag-black", KIconLoader::NoGroup, KIconLoader::SizeSmallMedium ) );
    m_blacklistLabelItem.data()->setToolTip( i18n( "Add to blacklist" ) );
    m_blacklistLabelItem.data()->setOpacity( 0.0 );
    delete iconLoader;

    m_addLabelAnimation = new QPropertyAnimation( m_addLabelItem.data(), "opacity", this );
    m_addLabelAnimation.data()->setEasingCurve( QEasingCurve::InOutQuad );
    m_removeLabelAnimation = new QPropertyAnimation( m_removeLabelItem.data(), "opacity", this );
    m_removeLabelAnimation.data()->setEasingCurve( QEasingCurve::InOutQuad );
    m_listLabelAnimation = new QPropertyAnimation( m_listLabelItem.data(), "opacity", this );
    m_listLabelAnimation.data()->setEasingCurve( QEasingCurve::InOutQuad );
    m_blacklistLabelAnimation = new QPropertyAnimation( m_blacklistLabelItem.data(), "opacity", this );
    m_blacklistLabelAnimation.data()->setEasingCurve( QEasingCurve::InOutQuad );

    setDeltaPointSize( deltaPointSize );
}

LabelGraphicsItem::~LabelGraphicsItem()
{}

void
LabelGraphicsItem::setDeltaPointSize( qreal deltaPointSize )
{
    QFont f = qApp->font();
    f.setPointSize( f.pointSizeF() + deltaPointSize );
    setFont( f );

    int iconsCount = 3;
    const int maxHeight = boundingRect().height() * 2 / 3;
    int maxWidth = ( boundingRect().width() - ( iconsCount - 1 ) * 2 ) / iconsCount;
    while( maxWidth < 14 )
    {
        iconsCount--;
        maxWidth = ( boundingRect().width() - ( iconsCount - 1 ) * 2 ) / iconsCount;
    }
    const int iconsSize = qMin( maxHeight, maxWidth );
    const int iconsSpaceA = ( boundingRect().width() - iconsSize * iconsCount ) / ( iconsCount - 1 );
    const int iconsSpaceB = iconsSize / 2;
    const int iconsSpace = qMin( iconsSpaceA, iconsSpaceB );

    m_addLabelItem.data()->setSize( iconsSize );
    m_addLabelItem.data()->setPos( 0, ( boundingRect().height() - iconsSize ) / 2 );
    m_removeLabelItem.data()->setSize( iconsSize );
    m_removeLabelItem.data()->setPos( 0, ( boundingRect().height() - iconsSize ) / 2 );
    m_listLabelItem.data()->setSize( iconsSize );
    m_listLabelItem.data()->setPos( iconsSize + iconsSpace, ( boundingRect().height() - iconsSize ) / 2 );
    m_listLabelItem.data()->setEnabled( iconsCount >= 2 );
    m_blacklistLabelItem.data()->setSize( iconsSize );
    m_blacklistLabelItem.data()->setPos( iconsSize * 2 + iconsSpace * 2, ( boundingRect().height() - iconsSize ) / 2 );
    m_blacklistLabelItem.data()->setEnabled( iconsCount >= 3 );
}

void
LabelGraphicsItem::setSelected( bool selected )
{
    m_selected = selected;
    setHoverValue( (float)isUnderMouse() );
    update();
}

void
LabelGraphicsItem::setSelectedColor( QColor color )
{
    m_selectedColor = color;
    setSelected( m_selected );
    update();
}

void
LabelGraphicsItem::updateHoverStatus()
{
    if( m_addLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_addLabelAnimation.data()->stop();
    if( m_removeLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_removeLabelAnimation.data()->stop();
    if( m_listLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_listLabelAnimation.data()->stop();
    if( m_blacklistLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_blacklistLabelAnimation.data()->stop();

    if( isUnderMouse() )
    {
        if( m_selected )
        {
            m_addLabelItem.data()->setOpacity( 0.0 );
            m_removeLabelItem.data()->setOpacity( 1.0 );
            m_removeLabelItem.data()->updateHoverStatus();
        }
        else
        {
            m_addLabelItem.data()->setOpacity( 1.0 );
            m_addLabelItem.data()->updateHoverStatus();
            m_removeLabelItem.data()->setOpacity( 0.0 );
        }

        if( m_listLabelItem.data()->isEnabled() )
        {
            m_listLabelItem.data()->setOpacity( 1.0 );
            m_listLabelItem.data()->updateHoverStatus();
        }

        if( m_blacklistLabelItem.data()->isEnabled() )
        {
            m_blacklistLabelItem.data()->setOpacity( 1.0 );
            m_blacklistLabelItem.data()->updateHoverStatus();
        }

        setHoverValue( 1.0 );
    }
    else
    {
        m_addLabelItem.data()->setOpacity( 0.0 );
        m_removeLabelItem.data()->setOpacity( 0.0 );
        m_listLabelItem.data()->setOpacity( 0.0 );
        m_blacklistLabelItem.data()->setOpacity( 0.0 );

        setHoverValue( 0.0 );
    }
}

void
LabelGraphicsItem::hoverEnterEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event )

    m_hoverValueAnimation.data()->setEndValue( 1.0 );
    m_hoverValueAnimation.data()->start();

    if( m_addLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_addLabelAnimation.data()->stop();
    if( m_removeLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_removeLabelAnimation.data()->stop();
    if( m_listLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_listLabelAnimation.data()->stop();
    if( m_blacklistLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_blacklistLabelAnimation.data()->stop();

    if( m_selected )
    {
        m_removeLabelAnimation.data()->setEndValue( 1.0 );
        m_removeLabelAnimation.data()->start();
    }
    else
    {
        m_addLabelAnimation.data()->setEndValue( 1.0 );
        m_addLabelAnimation.data()->start();
    }
    
    if( m_listLabelItem.data()->isEnabled() )
    {
        m_listLabelAnimation.data()->setEndValue( 1.0 );
        m_listLabelAnimation.data()->start();
    }

    if( m_blacklistLabelItem.data()->isEnabled() )
    {
        m_blacklistLabelAnimation.data()->setEndValue( 1.0 );
        m_blacklistLabelAnimation.data()->start();
    }
    
    update();
}

void
LabelGraphicsItem::hoverLeaveEvent( QGraphicsSceneHoverEvent *event )
{
    Q_UNUSED( event )
    
    if( m_addLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_addLabelAnimation.data()->stop();
    if( m_removeLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_removeLabelAnimation.data()->stop();
    if( m_listLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_listLabelAnimation.data()->stop();
    if( m_blacklistLabelAnimation.data()->state() != QAbstractAnimation::Stopped )
        m_blacklistLabelAnimation.data()->stop();

    if( m_selected )
    {
        m_removeLabelAnimation.data()->setEndValue( 0.0 );
        m_removeLabelAnimation.data()->start();
    }
    else
    {
        m_addLabelAnimation.data()->setEndValue( 0.0 );
        m_addLabelAnimation.data()->start();
    }

    if( m_listLabelItem.data()->isEnabled() )
    {
        m_listLabelAnimation.data()->setEndValue( 0.0 );
        m_listLabelAnimation.data()->start();
    }

    if( m_blacklistLabelItem.data()->isEnabled() )
    {
        m_blacklistLabelAnimation.data()->setEndValue( 0.0 );
        m_blacklistLabelAnimation.data()->start();
    }

    m_hoverValueAnimation.data()->setEndValue( 0.0 );
    m_hoverValueAnimation.data()->start();
    
    update();
}

void
LabelGraphicsItem::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    if( m_addLabelItem.data()->boundingRect().contains( mapToItem( m_addLabelItem.data(), event->pos() ) ) ||
        m_removeLabelItem.data()->boundingRect().contains( mapToItem( m_removeLabelItem.data(), event->pos() ) ) )
        emit toggled( toPlainText() );
    else if( m_listLabelItem.data()->isEnabled() && m_listLabelItem.data()->boundingRect().contains( mapToItem( m_listLabelItem.data(), event->pos() ) ) )
        emit list( toPlainText() );
    else if( m_blacklistLabelItem.data()->isEnabled() && m_blacklistLabelItem.data()->boundingRect().contains( mapToItem( m_blacklistLabelItem.data(), event->pos() ) ) )
        emit blacklisted( toPlainText() );
}

qreal
LabelGraphicsItem::hoverValue()
{
    return m_hoverValue;
}

void
LabelGraphicsItem::setHoverValue( qreal value )
{
    m_hoverValue = value;
    const QPalette p;
    const QColor c = p.color( QPalette::WindowText );
    const QColor defaultColor = m_selected ? m_selectedColor : c;

    const int red   = defaultColor.red()   + ( m_hoverColor.red()   - defaultColor.red()   ) * m_hoverValue;
    const int green = defaultColor.green() + ( m_hoverColor.green() - defaultColor.green() ) * m_hoverValue;
    const int blue  = defaultColor.blue()  + ( m_hoverColor.blue()  - defaultColor.blue()  ) * m_hoverValue;

    setDefaultTextColor( QColor( red, green, blue ) );
}

