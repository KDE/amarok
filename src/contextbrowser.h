// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information


#ifndef AMAROK_CONTEXTBROWSER_H
#define AMAROK_CONTEXTBROWSER_H

#include "engineobserver.h"
#include <qvbox.h>
#include <klistview.h>
#include <kparts/browserextension.h>
#include <kurl.h>

class CollectionDB;
class MetaBundle;
class QPalette;
class KHTMLPart;
class Scrobbler;

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
        void metaDataEdited( const MetaBundle &bundle );

    protected:
        void engineTrackEnded( int finalPosition, int trackLength );
        void engineNewMetaData( const MetaBundle&, bool );
        void engineStateChanged( Engine::State );
        void paletteChange( const QPalette& );

    private slots:
        void slotContextMenu( const QString& urlString, const QPoint& point );
        void showHome();
        void showCurrentTrack();
        void showLyrics();

        void lyricsData( KIO::Job* job, const QByteArray& data );
        void lyricsResult( KIO::Job* job );

        void relatedArtistsFetched( QStringList& artists );

    private:
        void setStyleSheet();
        void showIntroduction();
        void showScanning();
        void showCurrentStream();

        KHTMLPart *browser;
        MetaBundle *m_currentTrack;
        CollectionDB *m_db;
        Scrobbler *m_scrobbler;

        QString m_styleSheet;
        KURL m_url;
        QString m_artist;
        QString m_album;
        bool m_emptyDB;
        QString m_lyrics;
        QStringList m_relatedArtists;

        QString m_HTMLSource;
};

#endif /* AMAROK_CONTEXTBROWSER_H */
