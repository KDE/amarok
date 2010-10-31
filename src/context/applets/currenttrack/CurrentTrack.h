/****************************************************************************************
 * Copyright (c) 2007-2009 Leo Franchi <lfranchi@gmail.com>                             *
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2009 simon.esneault <simon.esneault@gmail.com>                         *
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

#ifndef CURRENT_TRACK_APPLET_H
#define CURRENT_TRACK_APPLET_H

#include "context/Applet.h"
#include "core/meta/Meta.h"
#include "ui_currentTrackSettings.h"

#include <Plasma/DataEngine>

class TextScrollingWidget;
class DropPixmapLayoutItem;
class RatingWidget;
class RecentlyPlayedListWidget;
class QAction;
class QGraphicsLinearLayout;

namespace Plasma {
    class Label;
}

static const KLocalizedString UNKNOWN_ARTIST = ki18n("Unknown Artist");
static const KLocalizedString UNKNOWN_ALBUM = ki18n("Unknown Album");

class CurrentTrack : public Context::Applet
{
    Q_OBJECT

public:
    CurrentTrack( QObject* parent, const QVariantList& args );
    ~CurrentTrack();

    virtual void paintInterface( QPainter *painter,
                                 const QStyleOptionGraphicsItem *option,
                                 const QRect &contentsRect );

public slots:
    virtual void init();
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data &data );

protected:
    virtual void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );
    void createConfigurationInterface( KConfigDialog *parent );

private slots:
    void trackRatingChanged( int rating );
    void connectSource( const QString &source );
    void paletteChanged( const QPalette &palette );
    void fontChanged();
    void coverDropped( const QPixmap &cover );
    void tracksCounted( QString id, QStringList results );
    void albumsCounted( QString id, QStringList results );
    void genresCounted( QString id, QStringList results );
    void queryCollection();

private:
    QList<QAction*> contextualActions();

    void clearTrackActions();
    void drawStatsBackground( QPainter *const p );
    void drawStatsTexts( QPainter *const p );
    void drawSourceEmblem( QPainter *const p );
    void resizeCover( const QPixmap &cover, qreal width );
    QPixmap amarokLogo( int dimension ) const;

    // aligns the second QGI to be at the same level as the first (the font baseline)
    void alignBaseLineToFirst( TextScrollingWidget *a, QGraphicsSimpleTextItem *b );

    QBrush normalBrush() const;
    QBrush unknownBrush() const;
    /**
     * Bug 205038
     * We check if original is an 'invalid' value
     * In that case we return replacement and
     * set widget's brush to unknownBrush()
     *
     * If original is 'valid', widget brush is set
     * to normalBrush() and original is returned
     */
    QString handleUnknown( const QString &original,
                           TextScrollingWidget *widget,
                           const QString &replacement );

    Plasma::Label *m_collectionLabel;
    RecentlyPlayedListWidget *m_recentWidget;
    RatingWidget *m_ratingWidget;
    DropPixmapLayoutItem *m_albumCover;
    TextScrollingWidget *m_recentHeader;
    TextScrollingWidget *m_title;
    TextScrollingWidget *m_artist;
    TextScrollingWidget *m_album;
    QGraphicsSimpleTextItem *m_byText;
    QGraphicsSimpleTextItem *m_onText;
    QGraphicsLinearLayout *m_actionsLayout;

    int m_rating;
    int m_score;
    int m_trackLength;
    int m_playCount;
    int m_trackCount;
    int m_albumCount;
    int m_genreCount;
    QDateTime m_lastPlayed;
    QString m_sourceEmblemPath;
    bool m_isStopped;

    Ui::currentTrackSettings ui_Settings;
    const int m_albumWidth;
};

K_EXPORT_AMAROK_APPLET( currenttrack, CurrentTrack )

#endif
