// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information

#ifndef AMAROK_CONTEXTBROWSER_H
#define AMAROK_CONTEXTBROWSER_H

#include "engineobserver.h"
#include <qvbox.h>

class CollectionDB;
class KHTMLPart;
class KTempFile;
class KToolBar;
class MetaBundle;
class QPalette;
class Scrobbler;
class Color;

namespace KIO { class Job; }

class ContextBrowser : public QVBox, public EngineObserver
{
    Q_OBJECT

    public:
        ContextBrowser( const char *name );
        ~ContextBrowser();

        virtual void setFont( const QFont& );

    public slots:
        void openURLRequest(const KURL &url );
        void collectionScanStarted();
        void collectionScanDone();

    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State );
        void paletteChange( const QPalette& );

    private slots:
        void slotContextMenu( const QString& urlString, const QPoint& point );
        void showHome();
        void showCurrentTrack();
        void showLyrics( const QString& hash = QString::null );
        void showLyricSuggestions();

        void lyricsData( KIO::Job* job, const QByteArray& data );
        void lyricsResult( KIO::Job* job );
        void coverFetched( const QString& keyword );

        void relatedArtistsFetched( QStringList& artists );

    private:
        void setStyleSheet();
        void showIntroduction();
        void showScanning();

        enum ToolBarID { Home, Lyrics, CurrentTrack };

        KHTMLPart    *browser;
        KToolBar     *m_toolbar;
        CollectionDB *m_db;
        Scrobbler    *m_scrobbler;

        QString       m_styleSheet;
        bool          m_emptyDB;
        QString       m_lyrics;
        QStringList   m_lyricSuggestions;
        QStringList   m_lyricHashes;
        QStringList   m_relatedArtists;
        QString       m_HTMLSource;
        KTempFile    *m_gradientImage;
        QStringList   m_metadataHistory;
};

#endif /* AMAROK_CONTEXTBROWSER_H */
