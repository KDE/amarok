// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Reigo Reinmets <xatax@hot.ee>
// (c) 2005 Mark Kretschmann <markey@web.de>
// (c) 2006 Peter C. Ndikuwera <pndiku@gmail.com>
// License: GNU General Public License V2


#ifndef AMAROK_CONTEXTBROWSER_H
#define AMAROK_CONTEXTBROWSER_H

#include "amarokdcophandler.h"
#include "clicklineedit.h"
#include "engineobserver.h"

#include <ktabwidget.h>
#include <ktoolbarbutton.h>
#include <kurl.h>

class ClickLineEdit;
class CollectionDB;
class Color;
class HTMLView;
class KPopupMenu;
class MetaBundle;
class QPalette;
class QVBox;
class QLineEdit;
class QComboBox;
class KDialogBase;
class KTabBar;
class KTextEdit;

class CueFile;

namespace Browser { class ToolBar; }
namespace KIO { class Job; class TransferJob; }


class ContextBrowser : public KTabWidget, public EngineObserver
{
    Q_OBJECT

        friend class CurrentTrackJob;
        friend class Amarok::DcopContextBrowserHandler;

        static ContextBrowser *s_instance;

    public:
        ContextBrowser( const char *name );
       ~ContextBrowser();

        static ContextBrowser *instance() { return s_instance; }

        void setFont( const QFont& );
        void reloadStyleSheet();
        static KURL::List expandURL( const KURL &url ); // expand urls (album, compilation, ...)
        static bool hasContextProtocol( const KURL &url ); // is url expandable by context browser?

        virtual bool eventFilter( QObject *o, QEvent *e ); //required by the labels dialog

    public slots:
        void openURLRequest(const KURL &url );
        void collectionScanStarted();
        void collectionScanDone( bool changed );
        void renderView();
        void lyricsChanged( const QString& );
        void lyricsScriptChanged();
        void lyricsResult( QCString cXmlDoc, bool cached = false );

    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State, Engine::State = Engine::Empty );
        void paletteChange( const QPalette& );

    protected slots:
        void wheelDelta( int delta );

    private slots:
        void tabChanged( QWidget *page );
        void slotContextMenu( const QString& urlString, const QPoint& point );
        void showContext( const KURL& url, bool fromHistory = false );
        void showCurrentTrack();
        void showLyrics( const QString& url = QString::null );
        void showWikipedia( const QString& url = QString::null, bool fromHistory = false, bool replaceHistory = false );
        void showWikipediaEntry( const QString& entry, bool replaceHistory = false );
        void reloadWikipedia();
        void showLabelsDialog();

        void coverFetched( const QString &artist, const QString &album );
        void coverRemoved( const QString &artist, const QString &album );
        void similarArtistsFetched( const QString &artist );
        void imageFetched( const QString &remoteURL );
        void tagsChanged( const MetaBundle &bundle );
        void tagsChanged( const QString &oldArtist, const QString &oldAlbum );
        void ratingOrScoreOrLabelsChanged( const QString &path );
        void refreshCurrentTrackPage();

        void contextHistoryBack();

        void lyricsAdd();
        void lyricsEditToggle();
        void lyricsSearch();
        void lyricsRefresh();
        void lyricsExternalPage();

	void lyricsSearchText( const QString &text );
	void lyricsSearchTextNext();
	void lyricsSearchTextHide();
	void lyricsSearchTextShow();
	void lyricsSearchTextToggle();

        void wikiHistoryBack();
        void wikiHistoryForward();
        void wikiBackPopupActivated( int id );
        void wikiForwardPopupActivated( int id );
        void wikiArtistPage();
        void wikiAlbumPage();
        void wikiTitlePage();
        void wikiExternalPage();
        void wikiResult( KIO::Job* job );
        void wikiConfigApply();
        void wikiConfig();
        void wikiConfigChanged( int activeItem );

    private:
        enum { CONTEXT_BACK, CONTEXT_FORWARD, CONTEXT_CURRENT, CONTEXT_HOME, CONTEXT_SEARCH };
        enum { LYRICS_ADD, LYRICS_EDIT, LYRICS_SEARCH, LYRICS_REFRESH, LYRICS_BROWSER };
        enum { WIKI_BACK, WIKI_FORWARD, WIKI_ARTIST, WIKI_ALBUM, WIKI_TITLE, WIKI_BROWSER, WIKI_CONFIG };
        typedef enum {SHOW_ALBUM_NORMAL, SHOW_ALBUM_SCORE, SHOW_ALBUM_LEAST_PLAY} T_SHOW_ALBUM_TYPE;
        static const uint WIKI_MAX_HISTORY = 20;
        static const uint CONTEXT_MAX_HISTORY = 20;

        void showIntroduction();
        void saveHtmlData();
        void showScanning();

        static QString getEncodedImage( const QString &imageUrl );

        static QString wikiLocale();
        static void setWikiLocale( const QString &locale );
        static QString wikiURL( const QString &item );
        QString wikiArtistPostfix() const;
        QString wikiAlbumPostfix() const;
        QString wikiTrackPostfix() const;

        HTMLView    *m_currentTrackPage;
        HTMLView    *m_lyricsPage;
        HTMLView    *m_wikiPage;

        QVBox        *m_contextTab;
        QVBox        *m_lyricsTab;
        QVBox        *m_wikiTab;
        // These controls are used to dictate whether the page should be rebuilt
        // true -> need rebuild
        bool          m_dirtyCurrentTrackPage;
        bool          m_dirtyLyricsPage;
        bool          m_dirtyWikiPage;

        QStringList   m_contextBackHistory;
        KURL          m_contextURL;

        QString       m_styleSheet;
        bool          m_emptyDB;
        QString       m_lyricAddUrl;
        QString       m_lyricSearchUrl;
        QString       m_lyricCurrentUrl;
        Browser::ToolBar* m_lyricsToolBar;
        KTextEdit*    m_lyricsTextEdit;
        QString       m_lyricsBeingEditedUrl;
        QString       m_lyricsBeingEditedArtist;
        QString       m_lyricsBeingEditedTitle;
        ClickLineEdit* m_lyricsSearchText;
        KToolBar*     m_lyricsTextBar;
        bool          m_lyricsTextBarShowed;


        QString       m_wiki;
        QString       m_wikiLanguages;
        static QString s_wikiLocale;
        QString       m_wikiBaseUrl;
        QString       m_wikiCurrentUrl;
        QString       m_wikiCurrentEntry;
        QStringList   m_wikiBackHistory;
        QStringList   m_wikiForwardHistory;
        KPopupMenu*   m_wikiBackPopup;
        KPopupMenu*   m_wikiForwardPopup;
        KIO::TransferJob* m_wikiJob;
        Browser::ToolBar* m_wikiToolBar;
        QLineEdit*    m_wikiLocaleEdit;
        QComboBox*    m_wikiLocaleCombo;
        KDialogBase*  m_wikiConfigDialog;

        QString       m_HTMLSource;
        QStringList   m_metadataHistory;
        KURL          m_currentURL;

        bool          m_relatedOpen;
        bool          m_suggestionsOpen;
        bool          m_favoritesOpen;
        bool          m_labelsOpen;
        bool          m_showRelated;
        bool          m_showSuggested;
        bool          m_showFaves;
        bool          m_showLabels;

        bool          m_showFreshPodcasts;
        bool          m_showFavoriteAlbums;
        bool          m_showNewestAlbums;

        bool          m_browseArtists;
        QString       m_artist;
        QStringList   m_shownAlbums;

        bool          m_browseLabels;
        QString       m_label;
        ClickLineEdit* m_addLabelEdit;
        QListView*    m_labelListView;

        CueFile      *m_cuefile;
};

#endif /* AMAROK_CONTEXTBROWSER_H */
