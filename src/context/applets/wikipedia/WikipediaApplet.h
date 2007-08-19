/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef WIKIPEDIA_APPLET_H
#define WIKIPEDIA_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"

class QGraphicsSimpleTextItem;
class QGraphicsTextItem;

class WikipediaApplet : public Context::Applet
{
    Q_OBJECT
public:
    WikipediaApplet( QObject* parent, const QStringList& args );
    
    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );
    
    // reimplemented to tell Plasma our size
    QSizeF contentSize() const;
    // reimplemented to help Plasma::Layout deal with us
    QSizeF sizeHint() const { return contentSize(); }
    
    void constraintsUpdated();
    
    void setRect( const QRectF& rect );
    // for use with the Context layout
    void setGeometry( const QRectF& rect ) { setRect( rect ); }
    
public slots:
    void updated( const QString& name, const Plasma::DataEngine::Data& data );
    
private:
    void calculateHeight();
    void resize( qreal newWidth, qreal aspectRatio );
    
    Context::Svg* m_theme;
    Context::Svg* m_header;
    qreal m_aspectRatio;
    qreal m_headerAspectRatio;
    QSizeF m_size;
    
    QGraphicsSimpleTextItem* m_wikipediaLabel;
    QGraphicsSimpleTextItem* m_currentLabel;
        
    QGraphicsTextItem* m_wikiPage;
    
};

K_EXPORT_AMAROK_APPLET( wikipedia, WikipediaApplet )
                        
#endif
