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

#include <QGraphicsProxyWidget>
#include <qwebview.h>

#include <plasma/framesvg.h>

class QAction;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;

namespace Plasma
{
    class WebView;
    class IconWidget;
}

static const int MARGIN = 10;

class WikipediaApplet : public Context::Applet
{
    Q_OBJECT
public:
    WikipediaApplet( QObject* parent, const QVariantList& args );
    ~WikipediaApplet();

    void init();
    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;
    
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    Plasma::IconWidget * addAction( QAction *action );

    Plasma::FrameSvg* m_theme;
    Context::Svg* m_header;
    qreal m_aspectRatio;
    qreal m_headerAspectRatio;
    QSizeF m_size;

    QGraphicsSimpleTextItem* m_wikipediaLabel;
    //QGraphicsSimpleTextItem* m_currentLabel;
    //QGraphicsSimpleTextItem* m_currentTitle;

    //QGraphicsProxyWidget* m_wikiPage;
    Plasma::WebView * m_webView;

    QString m_label;
    QString m_title;

    Plasma::IconWidget *m_reloadIcon;

private slots:
    void connectSource( const QString &source );
    void linkClicked( const QUrl &url );
    void reloadWikipedia();
};

K_EXPORT_AMAROK_APPLET( wikipedia, WikipediaApplet )

#endif
