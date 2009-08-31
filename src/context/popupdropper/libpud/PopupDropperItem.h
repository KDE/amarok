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

#ifndef POPUPDROPPERITEM_H
#define POPUPDROPPERITEM_H

#include <QtSvg/QGraphicsSvgItem>
#include <QString>
#include <QFont>

#include "PopupDropper_Export.h"

class QDropEvent;
class QGraphicsTextItem;
class QGraphicsView;
class QSvgRenderer;
class PopupDropper;
class PopupDropperItemPrivate;

class POPUPDROPPER_EXPORT PopupDropperItem : public QObject, public QAbstractGraphicsShapeItem
{
    Q_OBJECT

    Q_PROPERTY( HoverIndicatorShowStyle hoverIndicatorShowStyle READ hoverIndicatorShowStyle WRITE setHoverIndicatorShowStyle )
    Q_PROPERTY( Orientation orientation READ orientation WRITE setOrientation )
    Q_PROPERTY( TextProtection textProtection READ textProtection WRITE setTextProtection )
    Q_PROPERTY( QAction* action READ action WRITE setAction )
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( QFont font READ font WRITE setFont )
    Q_PROPERTY( QGraphicsTextItem* textItem READ textItem WRITE setTextItem )
    Q_PROPERTY( QGraphicsRectItem* borderRectItem READ borderRectItem WRITE setBorderRectItem )
    Q_PROPERTY( QGraphicsSvgItem* svgItem READ svgItem )
    Q_PROPERTY( QSvgRenderer* sharedRenderer READ sharedRenderer WRITE setSharedRenderer )
    Q_PROPERTY( QString elementId READ elementId WRITE setElementId )
    Q_PROPERTY( QRect svgElementRect READ svgElementRect WRITE setSvgElementRect )
    Q_PROPERTY( int horizontalOffset READ horizontalOffset WRITE setHorizontalOffset )
    Q_PROPERTY( int textOffset READ textOffset WRITE setTextOffset )
    Q_PROPERTY( int hoverMsecs READ hoverMsecs WRITE setHoverMsecs )
    Q_PROPERTY( int borderWidth READ borderWidth WRITE setBorderWidth )
    Q_PROPERTY( int hoverIndicatorRectWidth READ hoverIndicatorRectWidth WRITE setHoverIndicatorRectWidth )
    Q_PROPERTY( bool submenuTrigger READ submenuTrigger WRITE setSubmenuTrigger )
    Q_PROPERTY( QColor baseTextColor READ baseTextColor WRITE setBaseTextColor )
    Q_PROPERTY( QColor hoveredTextColor READ hoveredTextColor WRITE setHoveredTextColor )
    Q_PROPERTY( QPen hoveredBorderPen READ hoveredBorderPen WRITE setHoveredBorderPen )
    Q_PROPERTY( QBrush hoveredBrush READ hoveredFillBrush WRITE setHoveredFillBrush )
    Q_PROPERTY( QBrush hoverIndicatorBrush READ hoverIndicatorFillBrush WRITE setHoverIndicatorFillBrush )
    Q_PROPERTY( bool customBaseTextColor READ customBaseTextColor )
    Q_PROPERTY( bool customHoveredTextColor READ customHoveredTextColor )
    Q_PROPERTY( bool customHoveredBorderPen READ customHoveredBorderPen )
    Q_PROPERTY( bool customHoveredFillBrush READ customHoveredFillBrush )
    Q_PROPERTY( qreal subitemOpacity READ subitemOpacity WRITE setSubitemOpacity )
    Q_PROPERTY( bool separator READ isSeparator WRITE setSeparator )
    Q_PROPERTY( PopupDropperItem::SeparatorStyle separatorStyle READ separatorStyle WRITE setSeparatorStyle )
    Q_PROPERTY( bool hasLineSeparatorPen READ hasLineSeparatorPen )
    Q_PROPERTY( QPen lineSeparatorPen READ lineSeparatorPen WRITE setLineSeparatorPen )

public:
    enum HoverIndicatorShowStyle { Never, OnHover, AlwaysShow };
    Q_ENUMS( HoverIndicatorShowStyle )
    enum Orientation { Left, Right };
    Q_ENUMS( Orientation )
    enum TextProtection { NoProtection, MultiLine, ScaleFont };
    Q_ENUMS( TextProtection )
    enum SeparatorStyle{TextSeparator, LineSeparator};
    Q_ENUMS( separatorStyle )

    PopupDropperItem( QGraphicsItem *parent = 0 );
    explicit PopupDropperItem( const QString &file, QGraphicsItem *parent = 0 );
    virtual ~PopupDropperItem();

    void show();

    QAction* action() const;
    void setAction( QAction *action );

    HoverIndicatorShowStyle hoverIndicatorShowStyle() const;
    void setHoverIndicatorShowStyle( HoverIndicatorShowStyle hover );
    Orientation orientation() const;
    void setOrientation( Orientation orientation );
    TextProtection textProtection() const;
    void setTextProtection( TextProtection protection );

    QString text() const;
    void setText( const QString &text );
    QFont font() const;
    void setFont( const QFont &font );
    QColor baseTextColor() const;
    void setBaseTextColor( const QColor &color );
    QColor hoveredTextColor() const;
    void setHoveredTextColor( const QColor &color );
    QPen hoveredBorderPen() const;
    void setHoveredBorderPen( const QPen &pen );
    QBrush hoveredFillBrush() const;
    void setHoveredFillBrush( const QBrush &brush );
    QBrush hoverIndicatorFillBrush() const;
    void setHoverIndicatorFillBrush( const QBrush &brush );
    bool customBaseTextColor() const;
    bool customHoveredTextColor() const;
    bool customHoveredBorderPen() const;
    bool customHoveredFillBrush() const;
    void setSubitemOpacity( qreal opacity );
    qreal subitemOpacity() const;

    QGraphicsTextItem* textItem() const;
    void setTextItem( QGraphicsTextItem *textItem );
    void scaleAndReposSvgItem();
    void reposTextItem();
    void reposHoverFillRects();
    QGraphicsRectItem* borderRectItem() const;
    void setBorderRectItem( QGraphicsRectItem *borderRectItem );
    QGraphicsSvgItem* svgItem() const;

    QSvgRenderer* sharedRenderer() const;
    void setSharedRenderer( QSvgRenderer *renderer );
    QString elementId() const;
    void setElementId( const QString &id );
    QRect svgElementRect() const;
    void setSvgElementRect( const QRect &rect );
    int horizontalOffset() const;
    void setHorizontalOffset( int offset );
    int textOffset() const;
    void setTextOffset( int offset );

    bool isSeparator() const;
    void setSeparator( bool separator );
    PopupDropperItem::SeparatorStyle separatorStyle() const;
    void setSeparatorStyle( PopupDropperItem::SeparatorStyle style );
    bool hasLineSeparatorPen() const;
    QPen lineSeparatorPen() const;
    void setLineSeparatorPen( const QPen &pen );
    void clearLineSeparatorPen();

    int hoverMsecs() const;
    void setHoverMsecs( const int msecs );
    void hoverEntered();
    void hoverLeft();

    int borderWidth() const;
    void setBorderWidth( int width );
    int hoverIndicatorRectWidth() const;
    void setHoverIndicatorRectWidth( int width );

    bool submenuTrigger() const;
    void setSubmenuTrigger( bool trigger );

    void setPopupDropper( PopupDropper* pd );

    //bool operator<( const PopupDropperItem &other ) const;

    void fullUpdate();
    
    virtual QRectF boundingRect() const;
    virtual void paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0 );

public Q_SLOTS:
    virtual void dropped( QDropEvent *event );
    virtual void hoverFinished();
    virtual void hoverFrameChanged( int frame );

private:
    friend class PopupDropperItemPrivate;
    PopupDropperItemPrivate* const d;

};

#endif
