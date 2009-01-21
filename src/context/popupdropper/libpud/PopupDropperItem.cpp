/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "PopupDropper.h"
#include "PopupDropperItem.h"
#include "PopupDropperItem_p.h"
#include "PopupDropperAction.h"

#include <QtDebug>
#include <QtSvg/QSvgRenderer>
#include <QtSvg/QGraphicsSvgItem>
#include <QFont>

///////////////////////////////////////////////////////////

PopupDropperItemPrivate::PopupDropperItemPrivate( PopupDropperItem *parent )
    : action( 0 )
    , text( QString() )
    , hoverTimer( 500, parent )
    , elementId( QString() )
    , textItem( 0 )
    , borderRectItem( 0 )
    , svgItem( 0 )
    , hoverIndicatorRectItem( 0 )
    , hoverIndicatorRectFillItem( 0 )
    , borderWidth( 2 )
    , hoverIndicatorRectWidth( 15 )
    , font()
    , submenuTrigger( false )
    , baseTextColor()
    , hoveredTextColor()
    , hoveredBorderPen()
    , hoveredFillBrush()
    , hoverIndicatorRectFillBrush()
    , hoveredOver( false )
    , customBaseTextColor( false )
    , customHoveredTextColor( false )
    , customHoveredBorderPen( false )
    , customHoveredFillBrush( false )
    , separator( false )
    , file( QString() )
    , blankElementRect( 0, 0, 30, 50 )
    , horizontalOffset( 30 )
    , textOffset( 30 )
    , hoverIndicatorShowStyle( PopupDropperItem::Never )
    , orientation( PopupDropperItem::Left )
    , pd( 0 )
    , q( parent )
    {
        hoverTimer.setFrameRange( 0, 30 );
        q->setAcceptDrops( true );
        hoverIndicatorRectFillBrush.setColor( Qt::white );
        hoveredBorderPen.setWidth( borderWidth );
        hoveredBorderPen.setColor( Qt::white );
        hoveredBorderPen.setStyle( Qt::SolidLine );
        hoveredFillBrush.setColor( Qt::white );
        hoveredFillBrush.setStyle( Qt::SolidPattern );
    }

PopupDropperItemPrivate::~PopupDropperItemPrivate()
{
}

///////////////////////////////////////////////////////////

PopupDropperItem::PopupDropperItem( QGraphicsItem *parent )
    : QObject()
    , QAbstractGraphicsShapeItem( parent )
    , d( new PopupDropperItemPrivate( this ) )
{
    connect( &d->hoverTimer, SIGNAL( finished() ), this, SLOT( hoverFinished() ) );
    connect( &d->hoverTimer, SIGNAL( frameChanged(int) ), this, SLOT( hoverFrameChanged(int) ) );
}

PopupDropperItem::PopupDropperItem( const QString &file, QGraphicsItem *parent )
    : QObject()
    , QAbstractGraphicsShapeItem( parent )
    , d( new PopupDropperItemPrivate( this ) )
{
    d->file = file;
    connect( &d->hoverTimer, SIGNAL( finished() ), this, SLOT( hoverFinished() ) );
    connect( &d->hoverTimer, SIGNAL( frameChanged(int) ), this, SLOT( hoverFrameChanged(int) ) );
}

PopupDropperItem::~PopupDropperItem()
{
    //qDebug() << "Deleting popupdropperitem with text = " << d->text;
    delete d;
}

void PopupDropperItem::show()
{
}

PopupDropperAction* PopupDropperItem::action() const
{
    return d->action;
}

//warning: setting a PopupDropperAction will override any currently defined shared renderer
//and element id, if they exist in the action!
void PopupDropperItem::setAction( PopupDropperAction *action )
{
    //note that this also sets the text
    d->action = action;
    d->text = action->text();
    PopupDropperAction* pudaction = dynamic_cast<PopupDropperAction*>(action);
    if( pudaction )
    {
        if( !d->svgItem )
        {
            if( !d->file.isEmpty() )
                d->svgItem = new QGraphicsSvgItem( d->file, this );
            else
                d->svgItem = new QGraphicsSvgItem( this );
        }
        if( d->svgItem && pudaction->renderer() && pudaction->renderer()->isValid() )
            d->svgItem->setSharedRenderer( pudaction->renderer() );
        if( d->svgItem && !pudaction->elementId().isEmpty() && d->svgItem->renderer() && d->svgItem->renderer()->elementExists( pudaction->elementId() ) )
        {
            d->svgItem->setElementId( pudaction->elementId() );
            d->svgItem->show();
        }
        else if( d->svgItem )
            d->svgItem->hide();
        if( pudaction->isSeparator() )
            d->separator = true;
        reposSvgItem();

        d->hoverIndicatorRectItem = new QGraphicsRectItem( this );
            
        QPen pen = d->hoveredBorderPen;
        QColor color( pen.color() );
        color.setAlpha( 255 );
        pen.setColor( color );
        d->hoverIndicatorRectItem->setPen( pen );
        QBrush brush = d->hoverIndicatorRectItem->brush();
        brush.setStyle( Qt::NoBrush );
        d->hoverIndicatorRectItem->setBrush( brush );

        d->hoverIndicatorRectFillItem = new QGraphicsRectItem( this );
        pen = d->hoverIndicatorRectFillItem->pen();
        pen.setStyle( Qt::NoPen );
        d->hoverIndicatorRectFillItem->setPen( pen );
        d->hoverIndicatorRectFillBrush.setStyle( Qt::SolidPattern );

        if( d->hoverIndicatorShowStyle == PopupDropperItem::AlwaysShow )
            d->hoverIndicatorRectItem->show();
        else
            d->hoverIndicatorRectItem->hide();

        d->hoverIndicatorRectFillItem->hide();
    }

    if( d->pd )
        d->pd->updateAllOverlays();
}

PopupDropperItem::HoverIndicatorShowStyle PopupDropperItem::hoverIndicatorShowStyle() const
{
    return d->hoverIndicatorShowStyle;
}

void PopupDropperItem::setHoverIndicatorShowStyle( HoverIndicatorShowStyle hover )
{
    d->hoverIndicatorShowStyle = hover;
    if( !d->hoveredOver )
    {
        if( d->hoverIndicatorShowStyle == PopupDropperItem::AlwaysShow )
            d->hoverIndicatorRectItem->show();
        else
            d->hoverIndicatorRectItem->hide();
    }
}

PopupDropperItem::Orientation PopupDropperItem::orientation() const
{
    return d->orientation;
}

void PopupDropperItem::setOrientation( const Orientation orientation )
{
    d->orientation = orientation;
    reposSvgItem();
    reposTextItem();
    reposHoverFillRects();
}

QString PopupDropperItem::text() const
{
    return d->text;
}

void PopupDropperItem::setText( const QString &text )
{
    d->text = text;
}

QFont PopupDropperItem::font() const
{
    return d->font;
}

void PopupDropperItem::setFont( const QFont &font )
{
    d->font = font;
    if( d->textItem )
        d->textItem->setFont( font );
}

QColor PopupDropperItem::baseTextColor() const
{
    return d->baseTextColor;
}

void PopupDropperItem::setBaseTextColor( const QColor &color )
{
    if( !d->hoveredOver && d->textItem )
        d->textItem->setDefaultTextColor( color );
    d->baseTextColor = color;
    d->customBaseTextColor = true;
}

QColor PopupDropperItem::hoveredTextColor() const
{
    return d->hoveredTextColor;
}

void PopupDropperItem::setHoveredTextColor( const QColor &color )
{
    if( d->textItem && d->hoveredOver && d->hoverTimer.state() != QTimeLine::Running )
        d->textItem->setDefaultTextColor( color );
    d->hoveredTextColor = color;
    d->customHoveredTextColor = true; 
}

QPen PopupDropperItem::hoveredBorderPen() const
{
    return d->hoveredBorderPen;
}

void PopupDropperItem::setHoveredBorderPen( const QPen &pen )
{
    d->hoveredBorderPen = pen;
    d->customHoveredBorderPen = true;
    if( d->borderRectItem && ( !d->hoveredOver || ( d->hoveredOver && d->hoverTimer.state() != QTimeLine::Running ) ) )
    {
        QPen borderPen = pen;
    	if( !d->hoveredOver )
    	{
            QColor pencolor = borderPen.color();
            pencolor.setAlpha( 0 );
            borderPen.setColor( pencolor );
        }
        d->borderRectItem->setPen( borderPen );
    }
    if( d->hoverIndicatorRectItem && ( !d->hoveredOver || ( d->hoveredOver && d->hoverTimer.state() != QTimeLine::Running ) ) )
    {
        QPen borderPen = d->hoveredBorderPen;
        QColor color = borderPen.color();
        color.setAlpha( 255 );
        borderPen.setColor( color );
        d->hoverIndicatorRectItem->setPen( borderPen );
    }
}

QBrush PopupDropperItem::hoveredFillBrush() const
{
    return d->hoveredFillBrush;
}

void PopupDropperItem::setHoveredFillBrush( const QBrush &brush )
{
    d->hoveredFillBrush = brush;
    d->customHoveredFillBrush = true;
    if( d->borderRectItem && ( !d->hoveredOver || ( d->hoveredOver && d->hoverTimer.state() != QTimeLine::Running ) ) )
    {
        QBrush borderBrush = brush;
        if( !d->hoveredOver )
        {
            QColor brushColor = borderBrush.color();
            brushColor.setAlpha( 0 );
            borderBrush.setColor( brushColor );
        }
        d->borderRectItem->setBrush( borderBrush );
    }
}

QBrush PopupDropperItem::hoverIndicatorFillBrush() const
{
    return d->hoverIndicatorRectFillBrush;
}

void PopupDropperItem::setHoverIndicatorFillBrush( const QBrush &brush )
{
    d->hoverIndicatorRectFillBrush = brush;
    if( d->hoverIndicatorRectFillItem && ( d->hoveredOver && d->hoverTimer.state() != QTimeLine::Running ) )
        d->hoverIndicatorRectFillItem->setBrush( d->hoverIndicatorRectFillBrush );
}

bool PopupDropperItem::customBaseTextColor() const
{
    return d->customBaseTextColor;
}

bool PopupDropperItem::customHoveredTextColor() const
{
    return d->customHoveredTextColor;
}

bool PopupDropperItem::customHoveredBorderPen() const
{
    return d->customHoveredBorderPen;
}

bool PopupDropperItem::customHoveredFillBrush() const
{
    return d->customHoveredFillBrush;
}

QGraphicsTextItem* PopupDropperItem::textItem() const
{
    return d->textItem;
}

void PopupDropperItem::setTextItem( QGraphicsTextItem *textItem )
{
    d->textItem = textItem;
}

QGraphicsRectItem* PopupDropperItem::borderRectItem() const
{
    return d->borderRectItem;
}

void PopupDropperItem::setBorderRectItem( QGraphicsRectItem *borderRectItem )
{
    if( !borderRectItem )
        return;

    d->borderRectItem = borderRectItem;
    if( d->borderRectItem && !d->hoveredOver  )
    {
        QPen pen = d->hoveredBorderPen;
        QColor color = pen.color();
        color.setAlpha( 0 );
        pen.setColor( color );
        d->borderRectItem->setPen( pen );
        QBrush brush = d->hoveredFillBrush;
        color = brush.color();
        color.setAlpha( 0 );
        brush.setColor( color );
        d->borderRectItem->setBrush( brush );
    }
}

QGraphicsSvgItem* PopupDropperItem::svgItem() const
{
    return d->svgItem;
}

void PopupDropperItem::reposSvgItem()
{
    if( !d->svgItem )
        return;
    //qDebug() << "\n\nPUDItem boundingRect().width() = " << boundingRect().width();
    //qDebug() << "svgItem boundingRect width = " << d->svgItem->boundingRect().width();
    if( d->orientation == PopupDropperItem::Left )
    {
        d->svgItem->setPos( d->horizontalOffset, 0 );
        //qDebug() << "Left, svgItem pos = " << d->svgItem->pos().x();
    }
    else
    {
        //int rightside = d->borderRectItem ? d->borderRectItem->boundingRect().width() : boundingRect().width();
        int rightside;
        if( !d->pd || d->pd->viewSize().width() == 0 )
            rightside = boundingRect().width();
        else
            rightside = d->pd->viewSize().width();
        //qDebug() << "right side = " << rightside;
        d->svgItem->setPos( 
                rightside
                - d->svgItem->boundingRect().width()
                - d->horizontalOffset
                , 0 );
        //qDebug() << "Right, svgItem pos = " << d->svgItem->pos().x();
    }
    //qDebug() << "\n\n";
}

void PopupDropperItem::reposTextItem()
{
    int rightside; 
    if( !d->pd || d->pd->viewSize().width() == 0 )
        rightside = boundingRect().width();
    else
        rightside = d->pd->viewSize().width();
    if( d->textItem )
    {
        if( d->svgItem->elementId().isEmpty() || ( d->svgItem->renderer() && !d->svgItem->renderer()->elementExists( d->svgItem->elementId() ) ) )
        {
            int offsetPos = d->horizontalOffset + d->textOffset + d->blankElementRect.width();
            d->textItem->setPos(
                    ( d->orientation == PopupDropperItem::Left
                        ? offsetPos
                        : ( d->borderRectItem
                              ? d->borderRectItem->boundingRect().width() - offsetPos - d->textItem->boundingRect().width()
                              : rightside - offsetPos - d->textItem->boundingRect().width()
                          )
                    )
                , ( d->blankElementRect.height() / 2 ) - ( d->textItem->boundingRect().height() / 2 ) ); 
        }
        else
        {
            int offsetPos = d->horizontalOffset + d->textOffset + d->svgItem->boundingRect().width();
            d->textItem->setPos(
                    ( d->orientation == PopupDropperItem::Left
                        ? offsetPos
                        : ( d->borderRectItem
                            ? d->borderRectItem->boundingRect().width() - offsetPos - d->textItem->boundingRect().width()
                            : rightside - offsetPos - d->textItem->boundingRect().width()
                          )
                    )
                , ( d->svgItem->boundingRect().height() / 2 ) - ( d->textItem->boundingRect().height() / 2 ) );
        }
        d->textItem->setFont( d->font );
    }
}

void PopupDropperItem::reposHoverFillRects()
{
    if( !d->hoverIndicatorRectItem || !d->hoverIndicatorRectFillItem || !d->svgItem || !d->borderRectItem )
        return;

    //qDebug() << "\n\nPUDItem boundingRect().width() = " << boundingRect().width();
    qreal startx, starty, endx, endy;
    int rightside;
        if( !d->pd || d->pd->viewSize().width() == 0 )
            rightside = boundingRect().width();
        else
            rightside = d->pd->viewSize().width();
    //int rightside = d->borderRectItem ? d->borderRectItem->boundingRect().width() : boundingRect().width();
    if( d->orientation == PopupDropperItem::Left )
        startx = ( d->textItem
                     && ( d->svgItem->elementId().isEmpty()
                            || ( d->svgItem->renderer()
                                && !d->svgItem->renderer()->elementExists( d->svgItem->elementId() )
                               )
                        )
                 ? ( d->textItem->pos().x() / 2 )
                 : ( d->svgItem->pos().x() / 2 )
                 )
                 - ( d->hoverIndicatorRectWidth / 2 )
                 + d->hoverIndicatorRectItem->pen().width();
    else
    {
        //qDebug() << "right side = " << rightside;
        startx = ( d->textItem
                     && ( d->svgItem->elementId().isEmpty()
                            || ( d->svgItem->renderer()
                                && !d->svgItem->renderer()->elementExists( d->svgItem->elementId() )
                               )
                         )
                 ? ( rightside - ( ( rightside - d->textItem->pos().x() - d->textItem->boundingRect().width() ) / 2 ) )
                 : ( rightside - ( ( rightside - d->svgItem->pos().x() - d->svgItem->boundingRect().width() ) / 2 ) )
                 )
                 - ( d->hoverIndicatorRectWidth / 2 )
                 + d->hoverIndicatorRectItem->pen().width();
    }

    starty = ( d->svgItem->elementId().isEmpty() || ( d->svgItem->renderer() && !d->svgItem->renderer()->elementExists( d->svgItem->elementId() ) )
                ? ( d->borderRectItem->pos().y() + ( 2 * d->borderRectItem->pen().width() ) - ( d->hoverIndicatorRectItem->pen().width() ) )
                : ( d->svgItem->pos().y() + ( 2 * d->borderRectItem->pen().width() ) ) );

    endx = d->hoverIndicatorRectWidth - ( 2 * d->hoverIndicatorRectItem->pen().width() );

    endy = ( d->svgItem->elementId().isEmpty() || ( d->svgItem->renderer() && !d->svgItem->renderer()->elementExists( d->svgItem->elementId() ) )
                ? ( d->borderRectItem->boundingRect().height() - ( 4 * d->borderRectItem->pen().width() ) - ( 3 * d->hoverIndicatorRectItem->pen().width() ) )
                : ( d->svgItem->boundingRect().height() - ( 4 * d->borderRectItem->pen().width() ) ) );

    //qDebug() << "startx, endx = " << startx << ", " << endx;

    QRectF indicatorRect( startx, starty, endx, endy );
    d->hoverIndicatorRectItem->setRect( indicatorRect );

    QRectF indicatorFillRect(
            indicatorRect.left() + d->hoverIndicatorRectItem->pen().width()
            , indicatorRect.bottom() - d->hoverIndicatorRectItem->pen().width() 
            , indicatorRect.width() - ( 2 * d->hoverIndicatorRectItem->pen().width() )
            , 0 );
    d->hoverIndicatorRectFillItem->setRect( indicatorFillRect );
}

QSvgRenderer* PopupDropperItem::sharedRenderer() const
{
    if( d->svgItem )
        return d->svgItem->renderer();
    else
        return 0;
}

void PopupDropperItem::setSharedRenderer( QSvgRenderer *renderer )
{
    if( renderer && d->svgItem )
        d->svgItem->setSharedRenderer( renderer );
}

QString PopupDropperItem::elementId() const
{
    if( d->svgItem )
        return d->svgItem->elementId();
    else
        return QString();
}

void PopupDropperItem::setElementId( const QString &id )
{
    //qDebug() << "Element ID being set: " << id;
    if( d->svgItem && d->svgItem->renderer() && d->svgItem->renderer()->elementExists( id ))
    {
        d->svgItem->setElementId( id );
        d->svgItem->show();
        if( d->pd )
            d->pd->updateAllOverlays();
    }
}

QRect PopupDropperItem::blankElementRect() const
{
    return d->blankElementRect;
}

void PopupDropperItem::setBlankElementRect( const QRect &rect )
{
    d->blankElementRect = rect;
}

int PopupDropperItem::horizontalOffset() const
{
    return d->horizontalOffset;
}

void PopupDropperItem::setHorizontalOffset( int offset )
{
    d->horizontalOffset = offset;
}

int PopupDropperItem::textOffset() const
{
    return d->textOffset;
}

void PopupDropperItem::setTextOffset( int offset )
{
    d->textOffset = offset;
}

int PopupDropperItem::hoverMsecs() const
{
    return d->hoverTimer.duration();
}

void PopupDropperItem::setHoverMsecs( const int msecs )
{
    d->hoverTimer.setDuration( msecs );
}

void PopupDropperItem::hoverEntered()
{
    //qDebug() << "Starting hover timer for " << static_cast<QObject*>(this);
    if( d->hoverIndicatorRectItem && d->hoverIndicatorRectFillItem && d->hoverIndicatorShowStyle != PopupDropperItem::Never )
    {
        d->hoverIndicatorRectFillItem->show();
    }
    d->hoverTimer.stop();
    d->hoverTimer.setDirection( QTimeLine::Forward );
    d->hoverTimer.start();
    d->hoveredOver = true;
}

void PopupDropperItem::hoverLeft()
{
    //qDebug() << "Stopping hover timer for " << static_cast<QObject*>(this);
    d->hoverTimer.stop();
    d->hoverTimer.setDirection( QTimeLine::Backward );
    d->hoverTimer.start();
    d->hoveredOver = false;
}

int PopupDropperItem::borderWidth() const
{
    return d->borderWidth;
}

void PopupDropperItem::setBorderWidth( int borderWidth )
{
    d->borderWidth = borderWidth;
    d->hoveredBorderPen.setWidth( borderWidth );
    if( d->borderRectItem )
    {
        d->borderRectItem->setPen( d->hoveredBorderPen );
    }
}

int PopupDropperItem::hoverIndicatorRectWidth() const
{
    return d->hoverIndicatorRectWidth;
}

void PopupDropperItem::setHoverIndicatorRectWidth( int hoverIndicatorRectWidth )
{
    d->hoverIndicatorRectWidth = hoverIndicatorRectWidth;
    if( d->hoverIndicatorRectItem )
    {
        QPen pen = d->hoverIndicatorRectItem->pen();
        pen.setWidth( d->hoverIndicatorRectWidth );
        d->hoverIndicatorRectItem->setPen( pen );
    }
}

bool PopupDropperItem::submenuTrigger() const
{
    return d->submenuTrigger;
}

void PopupDropperItem::setSubmenuTrigger( bool trigger )
{
    d->submenuTrigger = trigger;
}

void PopupDropperItem::setPopupDropper( PopupDropper* pd )
{
    d->pd = pd;
}

//bool PopupDropperItem::operator<( const PopupDropperItem &other ) const
//{
//    return d->text < other.text();
//}

void PopupDropperItem::dropped( QDropEvent *event ) //virtual SLOT
{
    Q_UNUSED( event );
    d->hoverTimer.stop();
    //qDebug() << "PopupDropperItem drop detected";
    if( d->action )
    {
        //qDebug() << "Triggering action";
        d->action->activate( QAction::Trigger );
    }
}

void PopupDropperItem::hoverFinished() //SLOT
{
    //qDebug() << "PopupDropperItem timeout";
    if( d->separator )
        return;

    //qDebug() << d->hoverTimer.direction();;

    if( d->action && d->hoverTimer.direction() == QTimeLine::Forward )
        d->action->activate( QAction::Hover );
    
    if( d->hoverTimer.direction() == QTimeLine::Forward )
        d->textItem->setDefaultTextColor( d->hoveredTextColor );
    else
        d->textItem->setDefaultTextColor( d->baseTextColor );
   
    //Something is messed up in QTimeLine...I get hoverFinished immediately after doing a hoverLeft, but only sometimes...hence the check
    //to see if the timeline isn't running 
    if( d->hoverIndicatorRectFillItem && d->hoverTimer.state() == QTimeLine::NotRunning && d->hoverTimer.direction() == QTimeLine::Backward )
    {
        d->hoverIndicatorRectFillItem->hide();
        if( d->hoverIndicatorRectItem && d->hoverIndicatorShowStyle != PopupDropperItem::AlwaysShow )
            d->hoverIndicatorRectItem->hide();
    }
    
    if( d->pd )
        d->pd->updateAllOverlays();
}

void PopupDropperItem::hoverFrameChanged( int frame ) //SLOT
{
    if( d->separator )
        return;
    int range = d->hoverTimer.endFrame() - d->hoverTimer.startFrame();
    qreal multiplier = ( 1.0 * frame ) / range;
    int r = (int)( ( d->hoveredTextColor.red() - d->baseTextColor.red() ) * multiplier ) + d->baseTextColor.red();
    int g = (int)( ( d->hoveredTextColor.green() - d->baseTextColor.green() ) * multiplier ) + d->baseTextColor.green();
    int b = (int)( ( d->hoveredTextColor.blue() - d->baseTextColor.blue() ) * multiplier ) + d->baseTextColor.blue();
    int a = (int)( ( d->hoveredTextColor.alpha() - d->baseTextColor.alpha() ) * multiplier ) + d->baseTextColor.alpha();
    
    d->textItem->setDefaultTextColor( QColor( r, g, b, a ) );
    
    QColor borderColor = d->hoveredBorderPen.color();
    borderColor.setAlpha( (int)( borderColor.alpha() * multiplier ) );
    QPen pen = d->borderRectItem->pen();
    pen.setColor( borderColor );
    d->borderRectItem->setPen( pen );
    if( d->hoverIndicatorRectItem && d->hoverIndicatorShowStyle == PopupDropperItem::OnHover )
    {
        d->hoverIndicatorRectItem->setPen( pen );
        d->hoverIndicatorRectItem->show();
    }
    QColor fillColor = d->hoveredFillBrush.color();
    QBrush brush = d->borderRectItem->brush();
    fillColor.setAlpha( (int)( fillColor.alpha() * multiplier ) );
    brush.setColor( fillColor );
    d->borderRectItem->setBrush( brush );

    if( d->hoverIndicatorRectItem && d->hoverIndicatorRectFillItem && d->hoverIndicatorShowStyle != PopupDropperItem::Never )
    {
        int hoverIndicatorPenWidth = d->hoverIndicatorRectItem->pen().width();
        QRectF rect = d->hoverIndicatorRectFillItem->rect();
        QRectF outerRect = d->hoverIndicatorRectItem->rect();
        rect.setTop( ( multiplier * -1 * ( outerRect.bottom() - outerRect.top() - ( 2 * hoverIndicatorPenWidth ) ) ) 
                + outerRect.bottom() 
                - hoverIndicatorPenWidth );
        d->hoverIndicatorRectFillItem->setRect( rect );
        d->hoverIndicatorRectFillItem->setBrush( d->hoverIndicatorRectFillBrush );
        d->hoverIndicatorRectFillItem->show();
        //qDebug() << "hoverIndicatorRectFillItem = " << d->hoverIndicatorRectFillItem;
    }

    if( d->pd )
        d->pd->updateAllOverlays();
}

QRectF PopupDropperItem::boundingRect() const
{
    if( d->borderRectItem )
        return d->borderRectItem->boundingRect();
    else
        return QRectF( 0, 0, d->blankElementRect.width(), d->blankElementRect.height() );
}

void PopupDropperItem::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Q_UNUSED( painter )
    Q_UNUSED( option )
    Q_UNUSED( widget )
    return;
}

#include "PopupDropperItem.moc"

