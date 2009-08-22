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

#include <QtDebug>
#include <QtSvg/QSvgRenderer>
#include <QtSvg/QGraphicsSvgItem>
#include <QAction>
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
    , subitemOpacity( 0.0 )
    , file( QString() )
    , svgElementRect( 0, 0, 50, 50 )
    , sharedRenderer( 0 )
    , horizontalOffset( 30 )
    , textOffset( 30 )
    , separator( false )
    , hasLineSeparatorPen( false )
    , lineSeparatorPen()
    , hoverIndicatorShowStyle( PopupDropperItem::Never )
    , orientation( PopupDropperItem::Left )
    , textProtection( PopupDropperItem::ScaleFont )
    , separatorStyle( PopupDropperItem::TextSeparator )
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
    delete d;
}

void PopupDropperItem::show()
{
}

QAction* PopupDropperItem::action() const
{
    return d->action;
}

void PopupDropperItem::setAction( QAction *action )
{
    if( !action )
        return;
    //note that this also sets the text
    d->action = action;
    d->text = action->text();
    if( action )
    {
        if( !d->svgItem )
        {
            if( !d->file.isEmpty() )
                d->svgItem = new QGraphicsSvgItem( d->file, this );
            else
                d->svgItem = new QGraphicsSvgItem( this );
        }

        if( d->sharedRenderer )
            d->svgItem->setSharedRenderer( d->sharedRenderer );

        if( d->elementId.isEmpty() )
            d->elementId = action->property( "popupdropper_svg_id" ).toString();
        if( !d->elementId.isEmpty() )
        {
            if( d->svgItem->renderer() && d->svgItem->renderer()->elementExists( d->elementId ) )
                d->svgItem->setElementId( d->elementId );
        }

        if( !d->svgItem->elementId().isEmpty() && d->svgItem->renderer()->elementExists( d->svgItem->elementId() ) )
            d->svgItem->show();
        else
            d->svgItem->hide();
 
        if( action->isSeparator() )
            d->separator = true;
        
        scaleAndReposSvgItem();

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

        reposHoverFillRects();
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
    fullUpdate();
}

PopupDropperItem::TextProtection PopupDropperItem::textProtection() const
{
    return d->textProtection;
}

void PopupDropperItem::setTextProtection( const TextProtection textProtection )
{
    d->textProtection = textProtection;
    fullUpdate();
}

QString PopupDropperItem::text() const
{
    return d->text;
}

void PopupDropperItem::setText( const QString &text )
{
    d->text = text;
    if( d->textItem )
        d->textItem->setHtml( text );
    reposTextItem();
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
    reposTextItem();
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

qreal PopupDropperItem::subitemOpacity() const
{
    return d->subitemOpacity;
}

void PopupDropperItem::setSubitemOpacity( qreal opacity )
{
#if QT_VERSION >= 0x040500
    if( d->svgItem )
        d->svgItem->setOpacity( opacity );
    if( d->textItem )
        d->textItem->setOpacity( opacity );
    if( d->borderRectItem )
        d->borderRectItem->setOpacity( opacity );
    if( d->hoverIndicatorRectItem )
        d->hoverIndicatorRectItem->setOpacity( opacity );
    if( d->hoverIndicatorRectFillItem )
        d->hoverIndicatorRectFillItem->setOpacity( opacity );
#endif
}

QGraphicsTextItem* PopupDropperItem::textItem() const
{
    return d->textItem;
}

void PopupDropperItem::setTextItem( QGraphicsTextItem *textItem )
{
    d->textItem = textItem;
    if( d->textItem )
        d->textItem->setHtml( d->text );   
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
    if( !d->hoveredOver  )
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

void PopupDropperItem::scaleAndReposSvgItem()
{
    if( !d->svgItem || !d->borderRectItem )
        return;

    if( d->separator )
    {
        d->svgItem->scale( 0, 0 );
        d->svgItem->setPos( 0, 0 );
        return;
    }

    //Need to scale if it is too tall or wide
    qreal maxheight = d->svgElementRect.height() - ( 2 * d->borderRectItem->pen().width() );
    qreal maxwidth = d->svgElementRect.width() - ( 2 * d->borderRectItem->pen().width() );
    qreal vertScaleValue = maxheight / d->svgItem->sceneBoundingRect().height();
    qreal horizScaleValue = maxwidth / d->svgItem->sceneBoundingRect().width();
    qreal scaleValue = vertScaleValue < horizScaleValue ? vertScaleValue : horizScaleValue;
    
    d->svgItem->scale( scaleValue, scaleValue );

    qreal item_center = ( d->borderRectItem->sceneBoundingRect().height() / 2 ) + d->borderRectItem->pos().y();

    if( d->orientation == PopupDropperItem::Left )
    {
        d->svgItem->setPos( d->horizontalOffset, item_center - ( d->svgElementRect.height() / 2 ) );
    }
    else
    {
        int rightside;
        if( !d->pd || d->pd->viewSize().width() == 0 )
            rightside = sceneBoundingRect().width();
        else
            rightside = d->pd->viewSize().width();
            d->svgItem->setPos( 
                rightside
                - d->svgItem->sceneBoundingRect().width()
                - d->horizontalOffset
                , item_center - ( d->svgElementRect.height() / 2 ) );
    }
}

void PopupDropperItem::reposTextItem()
{
    if( !d->textItem || !d->borderRectItem )
        return;
 
    d->textItem->setFont( d->font );

    qreal item_vert_center = ( d->borderRectItem->sceneBoundingRect().height() / 2 ) + d->borderRectItem->pos().y();

    if( d->separator )
    {
        if( d->text.isEmpty() )
            return;
        qreal width = d->textItem->textWidth();
        if( width > d->borderRectItem->sceneBoundingRect().width() )
            d->textItem->setTextWidth( d->borderRectItem->sceneBoundingRect().width() );
        qreal offset = ( d->borderRectItem->sceneBoundingRect().width() - width ) / 2;
        d->textItem->setPos( offset, item_vert_center - ( d->textItem->sceneBoundingRect().height()  / 2 ) );
        return;
    }

    int rightside;
    if( !d->pd || d->pd->viewSize().width() == 0 )
        rightside = sceneBoundingRect().width();
    else
        rightside = d->pd->viewSize().width();
    
    int offsetPos = d->horizontalOffset + d->textOffset + d->svgElementRect.width();
    d->textItem->setPos(
            ( d->orientation == PopupDropperItem::Left
                ? offsetPos
                : 0
            )
        , item_vert_center - ( d->textItem->sceneBoundingRect().height() / 2 ) ); 

    if( d->textProtection == PopupDropperItem::ScaleFont )
    {
        QFontMetrics fm( d->textItem->font() );
        qreal desiredWidth = d->borderRectItem->sceneBoundingRect().width() - offsetPos;
        while( d->textItem->font().pointSize() > 1 &&
                ( fm.width( d->textItem->toPlainText() ) > desiredWidth ||
                  fm.height() > d->textItem->boundingRect().height() ) ) 
        {
            QFont font = d->textItem->font();
            font.setPointSize( font.pointSize() - 1 );
            d->textItem->setFont( font );
            fm = QFontMetrics( font );
        }
    }
    else if( d->textProtection == PopupDropperItem::MultiLine &&
             ( d->textItem->textWidth() == -1 ||
               d->textItem->textWidth() > ( d->borderRectItem->sceneBoundingRect().width() - offsetPos )
             )
           )
    {
        d->textItem->setTextWidth( d->borderRectItem->sceneBoundingRect().width() - offsetPos );
        reposTextItem();
    }
}

void PopupDropperItem::reposHoverFillRects()
{
    if( !d->hoverIndicatorRectItem || !d->hoverIndicatorRectFillItem || !d->textItem || !d->borderRectItem )
        return;

    if( d->separator )
    {
        d->hoverIndicatorRectItem->setRect( 0, 0, 0, 0 );
        d->hoverIndicatorRectFillItem->setRect( 0, 0, 0, 0 );
        return;
    }

    //qDebug() << "\n\nPUDItem boundingRect().width() = " << boundingRect().width();
    qreal startx, starty, endx, endy, item_center;
    int rightside;
        if( !d->pd || d->pd->viewSize().width() == 0 )
            rightside = sceneBoundingRect().width();
        else
            rightside = d->pd->viewSize().width();
    //int rightside = d->borderRectItem ? d->borderRectItem->boundingRect().width() : boundingRect().width();
    if( d->orientation == PopupDropperItem::Left )
        startx = d->horizontalOffset 
                 - d->hoverIndicatorRectWidth
                 - ( 2  * d->hoverIndicatorRectItem->pen().width() );
    else
    {
        //qDebug() << "right side = " << rightside;
        startx = rightside - d->horizontalOffset
                 + d->hoverIndicatorRectWidth
                 - ( 2 * d->hoverIndicatorRectItem->pen().width() );
    }

    item_center = ( d->borderRectItem->sceneBoundingRect().height() / 2 ) + d->borderRectItem->pos().y();

    starty = item_center - ( d->svgElementRect.height() / 2 );

    endx = d->hoverIndicatorRectWidth - ( 2 * d->hoverIndicatorRectItem->pen().width() );

    endy = d->svgElementRect.height();

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
    return d->sharedRenderer;
}

void PopupDropperItem::setSharedRenderer( QSvgRenderer *renderer )
{
    d->sharedRenderer = renderer;
    if( renderer && d->svgItem )
    {
        d->svgItem->setSharedRenderer( renderer );
        if( !d->svgItem->elementId().isEmpty() && d->svgItem->renderer()->elementExists( d->svgItem->elementId() ) )
        {
            d->svgItem->show();
            fullUpdate();
        }
    }
}

QString PopupDropperItem::elementId() const
{
    return d->elementId;
}

void PopupDropperItem::setElementId( const QString &id )
{
    //qDebug() << "Element ID being set: " << id;
    d->elementId = id;
    if( id.isEmpty() )
    {
        d->svgItem->hide();
        fullUpdate();
    }
    else if( d->svgItem && d->svgItem->renderer() && d->svgItem->renderer()->elementExists( id ))
    {
        d->svgItem->setElementId( id );
        d->svgItem->show();
        fullUpdate();
    }
}

QRect PopupDropperItem::svgElementRect() const
{
    return d->svgElementRect;
}

void PopupDropperItem::setSvgElementRect( const QRect &rect )
{
    d->svgElementRect = rect;
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

bool PopupDropperItem::isSeparator() const
{
    return d->separator;
}

void PopupDropperItem::setSeparator( bool separator )
{
    d->separator = separator;
}

PopupDropperItem::SeparatorStyle PopupDropperItem::separatorStyle() const
{
    return d->separatorStyle;
}

void PopupDropperItem::setSeparatorStyle( SeparatorStyle style )
{
    d->separatorStyle = style;
}

bool PopupDropperItem::hasLineSeparatorPen() const
{
    return d->hasLineSeparatorPen;
}

QPen PopupDropperItem::lineSeparatorPen() const
{
    return d->lineSeparatorPen;
}

void PopupDropperItem::setLineSeparatorPen( const QPen &pen )
{
    d->lineSeparatorPen = pen;
}

void PopupDropperItem::clearLineSeparatorPen()
{
    d->lineSeparatorPen = QPen();
    d->hasLineSeparatorPen = false;
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
    if( d->hoverIndicatorRectItem && d->hoverIndicatorRectFillItem && d->hoverIndicatorShowStyle != PopupDropperItem::Never )
    {
        d->hoverIndicatorRectFillItem->show();
    }
    d->hoverTimer.stop();
    d->hoverTimer.setDirection( QTimeLine::Forward );
    d->hoveredOver = true;
    d->hoverTimer.start();
}

void PopupDropperItem::hoverLeft()
{
    d->hoverTimer.stop();
    d->hoverTimer.setDirection( QTimeLine::Backward );
    d->hoveredOver = false;
    if( d->hoverTimer.currentFrame() != 0 )
        d->hoverTimer.start();
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
    if( d->separator )
        return;

    //qDebug() << "direction = forwards ? " << ( d->hoverTimer.direction() == QTimeLine::Forward ? "yes" : "no" );

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
    //qDebug() << "hoverFrameChanged for " << static_cast<QObject*>(this) << ", frame = " << frame;
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

void PopupDropperItem::fullUpdate()
{
    scaleAndReposSvgItem();
    reposTextItem();
    reposHoverFillRects();
    if( d->pd )
        d->pd->updateAllOverlays();
}

QRectF PopupDropperItem::boundingRect() const
{
    if( d->borderRectItem )
        return d->borderRectItem->boundingRect();
    else if( d->pd && d->pd->viewSize().width() != 0 )
        return QRectF( 0, 0, d->pd->viewSize().width(), d->svgElementRect.height() );
    else
        return QRectF( 0, 0, d->svgElementRect.width(), d->svgElementRect.height() );
}

void PopupDropperItem::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Q_UNUSED( painter )
    Q_UNUSED( option )
    Q_UNUSED( widget )
    return;
}

#include "PopupDropperItem.moc"

