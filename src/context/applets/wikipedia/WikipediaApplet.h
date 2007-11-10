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
    WikipediaApplet( QObject* parent, const QVariantList& args );

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );

    void constraintsUpdated();

public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    void calculateHeight();
    void resizeApplet( qreal newWidth, qreal aspectRatio );

    Context::Svg* m_theme;
    Context::Svg* m_header;
    qreal m_headerAspectRatio;

    QGraphicsSimpleTextItem* m_wikipediaLabel;
    QGraphicsSimpleTextItem* m_currentLabel;

    QGraphicsTextItem* m_wikiPage;

};

K_EXPORT_AMAROK_APPLET( wikipedia, WikipediaApplet )

#endif
