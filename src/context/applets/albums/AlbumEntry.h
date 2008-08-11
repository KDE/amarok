/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#ifndef ALBUM_ENTRY_H
#define ALBUM_ENTRY_H

#include <context/Svg.h>
#include <context/widgets/ToolBoxIcon.h>

#include <QGraphicsItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>

/**
 * A class to interact with albums entries
 */
class AlbumEntry: public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    explicit AlbumEntry( QGraphicsItem *parent=0 );
    QPainterPath shape() const;
    QRectF boundingRect() const;

    void setAlbumName( QString trackName );
    void setCoverImage( const QPixmap &coverImage );
    void setTrackCount( const QString &trackCount );

    QString albumName() const;
    const QPixmap &coverImage() const;
    QString trackCount() const;

    void resize( QSizeF newSize );
    QSizeF size() const;
    
protected:
    virtual void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0 );
    virtual void mousePressEvent( QGraphicsSceneMouseEvent *event ) {};
    virtual void mouseReleaseEvent( QGraphicsSceneMouseEvent *event ) {};
    virtual void hoverEnterEvent( QGraphicsSceneHoverEvent *event );
    virtual void hoverLeaveEvent( QGraphicsSceneHoverEvent *event );
    
private slots:
    void animateHighlight( qreal progress );
    void addToPlaylist();
    
private:
    bool     m_hovering;
    QString  m_trackName;
    QPixmap  m_coverImage;
    int      m_trackCount;

    Context::Svg* m_theme;
    
    QGraphicsSimpleTextItem  *m_albumName;
    QGraphicsSimpleTextItem  *m_trackCountString;
    QGraphicsPixmapItem      *m_cover;

    QSizeF m_size;
    int    m_coverWidth;

    int m_animHighlightId;

    qreal m_animHighlightFrame;

    ToolBoxIcon *m_addIcon;
    
Q_SIGNALS:
    void clicked( const QString &albumName );
};

#endif
