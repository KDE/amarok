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
class QGraphicsProxyWidget;
class QTextEdit;

class LyricsApplet : public Context::Applet
{
    Q_OBJECT
public:
    LyricsApplet( QObject* parent, const QVariantList& args );
    ~LyricsApplet();

    void init();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );
    bool hasHeightForWidth() const;

    void constraintsUpdated( Plasma::Constraints constraints = Plasma::AllConstraints );
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    void calculateHeight();

    Context::Svg* m_header;

    // holds main body
    QGraphicsProxyWidget *m_lyricsProxy;
    QTextEdit* m_lyrics;
};

K_EXPORT_AMAROK_APPLET( lyrics, LyricsApplet )

#endif
