/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_WIKIPEDIA_ENGINE
#define AMAROK_WIKIPEDIA_ENGINE

#include "core/meta/Meta.h"
#include "network/NetworkAccessManagerProxy.h"

#include <QObject>
#include <QPalette>


class WikipediaEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString page READ page NOTIFY pageChanged)
    Q_PROPERTY(QUrl url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(SelectionType selection READ selection WRITE setSelection NOTIFY selectionChanged)
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)

public:
    enum SelectionType
    {
        Artist,
        Composer,
        Album,
        Track
    };
    Q_ENUM(SelectionType)

    explicit WikipediaEngine( QObject* parent = nullptr );
    ~WikipediaEngine() override;

    QString page() const { return m_page; }
    QUrl url() const { return wikiCurrentUrl; }
    void setUrl( const QUrl &url );
    QString message() const { return m_message; }
    void setMessage( const QString &message );
    bool busy() const { return m_busy; }
    SelectionType selection() const;
    bool setSelection( SelectionType type ); // returns true if selection is changed
    QString title() const { return m_title; }
    QString language() const { return preferredLangs.first(); }
    void setLanguage( const QString &language );
    Q_INVOKABLE void reloadWikipedia();

Q_SIGNALS:
    void pageChanged();
    void messageChanged();
    void busyChanged();
    void selectionChanged();
    void languageChanged();
    void titleChanged();
    void urlChanged();

private:
    void fetchWikiUrl( const QString &title, const QString &urlPrefix );
    void fetchLangLinks( const QString &title, const QString &hostLang, const QString &llcontinue = QString() );
    void fetchListing( const QString &title, const QString &hostLang );
    bool setSelection( const QString &type );
    void updateEngine();
    void wikiParse( QString &page );
    QString createLanguageComboBox( const QMap<QString, QString> &languageMap );
    void setPage( const QString &page );
    void setBusy( bool busy );
    void setTitle( const QString &title );
    void clear();

    SelectionType currentSelection;
    QUrl wikiCurrentUrl;
    QStringList preferredLangs;
    struct TrackMetadata
    {
        QString artist;
        QString composer;
        QString album;
        QString track;
        void clear()
        {
            artist.clear();
            composer.clear();
            album.clear();
            track.clear();
        }
    } m_previousTrackMetadata;
    bool useMobileVersion;

    QSet< QUrl > urls;
    QString m_page;
    QString m_message;
    bool m_busy;
    QString m_title;
    QString m_css;

private Q_SLOTS:
    void _checkRequireUpdate( Meta::TrackPtr track );
    void _parseLangLinksResult(const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );
    void _parseListingResult(const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );
    void _wikiResult(const QUrl &url, const QByteArray &result, const NetworkAccessManagerProxy::Error &e );
    void _stopped();
    void _paletteChanged( const QPalette &palette );
};


#endif

