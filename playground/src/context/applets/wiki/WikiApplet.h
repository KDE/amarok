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

#ifndef WIKI_APPLET_H
#define WIKI_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"

#include <ktemporaryfile.h>
#include <plasma/framesvg.h>

#include <QGraphicsProxyWidget>
#include <QGraphicsSceneContextMenuEvent>
#include <plasma/widgets/webview.h>
#include <qwebview.h>

class QAction;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;

namespace Plasma
{
    class WebView;
    class IconWidget;
}

class MyWebView;

class WikiApplet : public Context::Applet
{
    Q_OBJECT
public:
    WikiApplet( QObject* parent, const QVariantList& args );
    ~WikiApplet();

    void init();
    void initLyrics();
    void initWikipedia();
    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

    virtual QSizeF sizeHint( Qt::SizeHint which, const QSizeF & constraint) const;
    
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    Plasma::IconWidget * addAction( QAction *action );

    qreal m_aspectRatio;
    qreal m_headerAspectRatio;
    QSizeF m_size;

    QGraphicsSimpleTextItem* m_wikipediaLabel;
    //QGraphicsSimpleTextItem* m_currentLabel;
    //QGraphicsSimpleTextItem* m_currentTitle;

    //QGraphicsProxyWidget* m_wikiPage;
    MyWebView * m_webView;

    QString m_label;

    QString m_lyricsTitle,m_artistTitle,m_albumTitle;

    QString m_artistHtml,m_albumHtml,m_lyricsHtml;
    QString m_compressedArtistHtml,m_compressedAlbumHtml,m_compressedLyricsHtml;
    
    bool m_artistState,m_albumState,m_lyricsState;

    Plasma::IconWidget *m_reloadIcon;

    KTemporaryFile* m_css;

    QMenu *m_contextMenu;

    int m_navigationFlag;
    int m_infoFlag;

    void updateArtistInfo(const Plasma :: DataEngine :: Data& data);
    void updateAlbumInfo(const Plasma :: DataEngine :: Data& data);
    void updateLyricsInfo(const Plasma :: DataEngine :: Data& data);

    void createContextMenu();

    void resetStates();


private slots:
    void connectSource( const QString &source );
    void linkClicked( const QUrl &url );
    void reloadWikipedia();
    void paletteChanged( const QPalette & palette );
    void updateHappening();

    void reloadArtistInfo();
    void reloadAlbumInfo();
    void reloadLyricsInfo();

    void navigateToArtist();
    void navigateToAlbum();
    void navigateToLyrics();

    void compressAlbumInfo();
    void compressArtistInfo();
    void compressLyricsInfo();

    void expandAlbumInfo();
    void expandArtistInfo();
    void expandLyricsInfo();

};

class MyWebView : public Plasma :: WebView
{
    Q_OBJECT
    QMenu *m_contextMenu;
    public:
        MyWebView(QGraphicsItem *parent = 0);
        void loadMenu(QMenu *menu);
        void toggleAction(const QString s,bool status);
        void enableViewActions(const QString s);
        void setDefaultActions(const QString s);
        void enableNavigateAction(const QString s);
	~MyWebView();
    protected:
        void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};


K_EXPORT_AMAROK_APPLET( wiki, WikiApplet )

#endif
