/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2009 Vignesh Chandramouli <vig.chan@gmail.com>                                                                                     *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef WIKI_APPLET_H
#define WIKI_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "context/Svg.h"
#include "meta/Meta.h"

#include <ktemporaryfile.h>
#include <plasma/framesvg.h>

#include <QGraphicsProxyWidget>
#include <QGraphicsSceneContextMenuEvent>
#include <plasma/widgets/webview.h>

#include <qwebview.h>
#include <ui_wikiSettings.h>

class QAction;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;
class KDialog;
class KConfigDialog;

namespace Plasma
{
    class WebView;
    class IconWidget;
    class BusyWidget;
}

class MyWebView;

class WikiApplet : public Context::Applet
{
    Q_OBJECT
public:
    WikiApplet( QObject* parent, const QVariantList& args );
    ~WikiApplet();

    void init();
    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option, const QRect& contentsRect );

    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    bool hasHeightForWidth() const;
    qreal heightForWidth( qreal width ) const;

protected:
    void createConfigurationInterface(KConfigDialog *parent);
    
public slots:
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:
    Plasma::IconWidget * addAction( QAction *action );

    qreal m_aspectRatio;
    qreal m_headerAspectRatio;
    QSizeF m_size;

    QGraphicsSimpleTextItem* m_wikipediaLabel;

    MyWebView * m_webView;

    QString m_label;

    QString m_lyricsTitle,m_artistTitle,m_albumTitle,m_trackTitle;

    QString m_artistHtml,m_albumHtml,m_lyricsHtml,m_titleHtml;
    QString m_compressedArtistHtml,m_compressedAlbumHtml,m_compressedLyricsHtml,m_compressedTitleHtml;

    QString m_current;
    
    QList <QString> m_histoBack;
    QList <QString> m_histoFor;

    QString m_wikiPreferredLang;
    
    bool m_artistState,m_albumState,m_lyricsState,m_titleState;
    int m_pageState;
    bool m_prevTrackInfoAvailable;

    Plasma::IconWidget *m_reloadIcon;
    Plasma::IconWidget *m_lyricsIcon,*m_artistIcon,*m_albumIcon,*m_titleIcon;
    Plasma::IconWidget *m_settingsIcon;
    Plasma::IconWidget *m_forwardIcon,*m_backwardIcon;
    Plasma::IconWidget *m_currentTrackIcon,*m_previousTrackIcon;

    Ui::wikiSettings ui_Settings;

    KTemporaryFile* m_css;

    QMenu *m_contextMenu;

    void updateArtistInfo( const Plasma :: DataEngine :: Data& data );
    void updateAlbumInfo( const Plasma :: DataEngine :: Data& data );
    void updateLyricsInfo( const Plasma :: DataEngine :: Data& data );
    void updateTitleInfo( const Plasma :: DataEngine :: Data& data );

    void createContextMenu();

    void resetStates();
    
private slots:
    void connectSource( const QString &source );
    void linkClicked( const QUrl &url );
    void reloadWikipedia();
    void paletteChanged( const QPalette & palette );
    void updateWebView();
    
    void switchLang();
    void switchToLang(QString lang);

    void goForward();
    void goBackward();
    
    void reloadArtistInfo();
    void reloadAlbumInfo();
    void reloadLyricsInfo();
    void reloadTitleInfo();

    void navigateToArtist();
    void navigateToAlbum();
    void navigateToLyrics();
    void navigateToTitle();

    void compressAlbumInfo();
    void compressArtistInfo();
    void compressLyricsInfo();
    void compressTitleInfo();

    void expandAlbumInfo();
    void expandArtistInfo();
    void expandLyricsInfo();
    void expandTitleInfo();

    void updateWebPageIcons();
    void viewCurrentTrackInfo();
    void viewPreviousTrackInfo();
    void enablePrevTrackIcon();
};

class MyWebView : public Plasma :: WebView
{
    Q_OBJECT

    QMenu *m_contextMenu;
    Plasma::BusyWidget *m_artistBusy;
    bool m_hideMenu;
    
    public:
        MyWebView(QGraphicsItem *parent = 0);
        void loadMenu(QMenu *menu);
        
        void toggleAction(const QString s,bool status);
        void enableViewActions(const QString s);
        void disableViewActions( const QString s);
        void setDefaultActions(const QString s);
        void enableNavigateAction(const QString s);
        void disableAllActions();
        void enablePreviousTrackAction( bool status );
        void enableCurrentTrackAction( bool status );
        void disableReloadActions( const QString s );

        ~MyWebView();
        
    protected:
        void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);
};


K_EXPORT_AMAROK_APPLET( wiki, WikiApplet )

#endif
