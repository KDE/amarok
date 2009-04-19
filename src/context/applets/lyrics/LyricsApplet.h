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

#include <plasma/widgets/iconwidget.h>


class QGraphicsSimpleTextItem;
class QGraphicsProxyWidget;
class QTextBrowser;

class LyricsApplet : public Context::Applet
{
    Q_OBJECT

public:
    LyricsApplet( QObject* parent, const QVariantList& args );
    ~LyricsApplet();

    virtual void init();

    bool hasHeightForWidth() const;

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;
    
public slots:
    void connectSource( const QString& source );
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );
    void suggestionChosen( const QString& link );
    void refreshLyrics();
    
private slots:
    void paletteChanged( const QPalette & palette );

private:
    void calculateHeight();
    Plasma::IconWidget* addAction( QAction *action );

    QString m_titleText;
    QGraphicsSimpleTextItem* m_titleLabel;
    Plasma::IconWidget* m_reloadIcon;
    
    // holds main body
    QGraphicsProxyWidget *m_lyricsProxy;
    QTextBrowser* m_lyrics;
    QGraphicsTextItem* m_suggested;
};

K_EXPORT_AMAROK_APPLET( lyrics, LyricsApplet )

#endif
