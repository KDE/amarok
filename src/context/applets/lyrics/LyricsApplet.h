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

#ifndef LYRICS_APPLET_H
#define LYRICS_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"

class QGraphicsSimpleTextItem;
class QGraphicsTextItem;

class LyricsApplet : public Context::Applet
{
    Q_OBJECT
public:
    LyricsApplet( QObject* parent, const QVariantList& args );

    void init();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );
    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

    void constraintsUpdated( Plasma::Constraints );
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    void calculateHeight();

    Context::Svg* m_header;
    qreal m_headerAspectRatio;

    // labels
    QGraphicsSimpleTextItem* m_lyricsLabel;
    QGraphicsSimpleTextItem* m_titleLabel;
    QGraphicsSimpleTextItem* m_artistLabel;
    QGraphicsSimpleTextItem* m_siteLabel;
    // holds main body
    QGraphicsTextItem* m_lyrics;
    // titles
    QGraphicsSimpleTextItem* m_title;
    QGraphicsSimpleTextItem* m_artist;
    QGraphicsSimpleTextItem* m_site;
};

K_EXPORT_AMAROK_APPLET( lyrics, LyricsApplet )

#endif
