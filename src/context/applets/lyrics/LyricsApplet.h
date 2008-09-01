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

    virtual void init();

    bool hasHeightForWidth() const;

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;
    
public slots:
    void connectSource( const QString& source );
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    void calculateHeight();

    // holds main body
    QGraphicsProxyWidget *m_lyricsProxy;
    QTextEdit* m_lyrics;
    QGraphicsTextItem* m_suggested;
    qreal m_aspectRatio;
};

K_EXPORT_AMAROK_APPLET( lyrics, LyricsApplet )

#endif
