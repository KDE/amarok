/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef AMAROK_LYRICS_ENGINE
#define AMAROK_LYRICS_ENGINE

#include "core/meta/Meta.h"

#include <QObject>
#include <QString>
#include <QVariantList>

class LyricsEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text NOTIFY lyricsChanged)
    Q_PROPERTY(bool fetching READ fetching NOTIFY fetchingChanged)
    Q_PROPERTY(QVariantList suggestions READ suggestions NOTIFY lyricsChanged)
    Q_PROPERTY(qreal position READ position NOTIFY positionChanged)
    Q_PROPERTY(qreal fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
    Q_PROPERTY(int alignment READ alignment WRITE setAlignment NOTIFY alignmentChanged)
    Q_PROPERTY(QString font READ font WRITE setFont NOTIFY fontChanged)

public:
    explicit LyricsEngine( QObject* parent = Q_NULLPTR );

    void newLyrics( const Meta::TrackPtr &track );
    void newSuggestions( const QVariantList &suggest );
    void lyricsMessage( const QString& key, const QString& val );

    QString text() const { return m_lyrics; }
    QVariantList suggestions() const { return m_suggestions; }
    bool fetching() const { return m_fetching; }
    qreal position() const;
    qreal fontSize() const;
    void setFontSize( qreal fontSize );
    int alignment() const;
    void setAlignment( int alignment );
    QString font() const;
    void setFont( const QString &font );

    Q_INVOKABLE void refetchLyrics() const;
    Q_INVOKABLE QStringList availableFonts() const;

Q_SIGNALS:
    void lyricsChanged();
    void newLyricsMessage( const QString& key, const QString &val );
    void positionChanged();
    void fetchingChanged();
    void fontSizeChanged();
    void alignmentChanged();
    void fontChanged();

private Q_SLOTS:
    void update();
    void onTrackMetadataChanged( Meta::TrackPtr track );

private:
    void clearLyrics();

    QString m_lyrics;
    QVariantList m_suggestions;
    bool m_fetching;
    bool m_isUpdateInProgress;

    struct trackMetadata {
        QString artist;
        QString title;
    } m_prevTrackMetadata;
};

#endif
