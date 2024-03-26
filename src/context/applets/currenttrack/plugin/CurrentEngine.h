/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#ifndef AMAROK_CURRENT_ENGINE
#define AMAROK_CURRENT_ENGINE

#include "core/meta/Meta.h"

#include <QObject>
#include <QPixmap>
#include <QVariant>

class CurrentEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString artist READ artist NOTIFY trackChanged)
    Q_PROPERTY(QString track READ track NOTIFY trackChanged)
    Q_PROPERTY(QString album READ album NOTIFY trackChanged)
    Q_PROPERTY(int rating READ rating WRITE setRating NOTIFY trackChanged)
    Q_PROPERTY(int score READ score NOTIFY trackChanged)
    Q_PROPERTY(int length READ length NOTIFY trackChanged)
    Q_PROPERTY(QString lastPlayed READ lastPlayed NOTIFY trackChanged)
    Q_PROPERTY(int timesPlayed READ timesPlayed NOTIFY trackChanged)
    Q_PROPERTY(QVariant cover READ cover NOTIFY albumChanged)
    Q_PROPERTY(bool hasValidCover READ hasValidCover NOTIFY albumChanged)

public:
    explicit CurrentEngine( QObject* parent = nullptr );
    ~CurrentEngine() override;

    QString artist() const;
    QString track() const;
    QString album() const;
    int rating() const;
    void setRating( int rating );
    int score() const;
    int length() const;
    QString lastPlayed() const;
    int timesPlayed() const;
    QVariant cover() const { return QVariant(m_cover); }
    bool hasValidCover() const { return !m_cover.isNull(); }
    Q_INVOKABLE void displayCover();

Q_SIGNALS:
    void trackChanged();
    void albumChanged();
    void coverWidthChanged();

private Q_SLOTS:
    void slotAlbumMetadataChanged(const Meta::AlbumPtr &album );
    void slotTrackMetadataChanged( Meta::TrackPtr track );
    void slotTrackChanged( const Meta::TrackPtr &track );
    void stopped();

private:
    void update( Meta::TrackPtr track );
    void update( Meta::AlbumPtr album );

    QPixmap m_cover;
    Meta::TrackPtr m_currentTrack;

private Q_SLOTS:

};


#endif
