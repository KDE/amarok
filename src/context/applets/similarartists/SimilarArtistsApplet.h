/****************************************************************************************
 * Copyright (c) 2009-2010 Joffrey Clavel <jclavel@clabert.info>                        *
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2010 Alexandre Mendes <alex.mendes1988@gmail.com>                      *
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

#ifndef SIMILAR_ARTISTS_APPLET_H
#define SIMILAR_ARTISTS_APPLET_H


#include "./ArtistWidget.h"
#include "./SimilarArtist.h"

//Amarok
#include "context/Applet.h"
#include "context/DataEngine.h"
#include "EngineObserver.h"

#include <ui_similarArtistsSettings.h>


class QAction;
class TextScrollingWidget;
class KConfigDialog;
class QVBoxLayout;
class QGraphicsProxyWidget;
class QScrollArea;

namespace Plasma
{
class IconWidget;
}


/**
 * SimilarArtists will display similar artists from the Internet, relative to the current playing artist.
 * @author Joffrey Clavel
 * @author Oleksandr Khayrullin
 * @version 0.1
 */
class SimilarArtistsApplet : public Context::Applet, public EngineObserver
{
    Q_OBJECT

public:

    /**
     * SimilarArtistsApplet constructor
     * @param parent The widget parent
     * @param args   List of strings containing two entries: the service id
     *               and the applet id
     */
    SimilarArtistsApplet( QObject* parent, const QVariantList& args );

    /**
     * SimilarArtistsApplet destructor
     */
    ~SimilarArtistsApplet();


    /**
     * Initialization of the applet's display, creation of the layout, scrolls
     */
    void init();

    void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem* option
                         , const QRect& contentsRect );
    /**
     * This method puts the widgets in the layout, in the initialization
     */
    void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

    // inherited from EngineObserver

    /**
     * This method was launch when amarok play a new track
     */
    virtual void engineNewTrackPlaying();

    /**
     * This method was launch when amarok stop is playback (ex: The user has clicked on the stop button)
     */
    virtual void enginePlaybackEnded( qint64 finalPosition, qint64 trackLength
                                      , PlaybackEndedReason reason );

protected:
    void createConfigurationInterface( KConfigDialog *parent );

public slots:

    /**
     * Update the current artist and his similar artists
     */
    void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );

private:

    /**
     * Update the display of the artists according to the lists m_similars
     */
    void artistsUpdate();

    qreal m_aspectRatio;
    qreal m_headerAspectRatio;
    QSizeF m_size;

    QString m_descriptionPreferredLang;

    /**
     * Layout for the formatting of the applet contents
     */
    QVBoxLayout *m_layout;

    /**
     * This proxy widget contents the scrollArea
     */
    QGraphicsProxyWidget *m_scrollProxy;

    /**
     * This scrollArea contents the artists widgets
     */
    QScrollArea             *m_scroll;

    /**
     * Indicates if a track is playing.
     */
    bool  m_stoppedState;

    /**
     * The list of similar artists to display
     */
    SimilarArtist::SimilarArtistsList m_similars;

    /**
     * Artist which you want to see artists like
     */
    QString m_artist;

    /**
     * Title of the applet (in the top bar)
     */
    TextScrollingWidget *m_headerLabel;

    //Icons on the title right
    Plasma::IconWidget *m_settingsIcon;
    Ui::similarArtistsSettings ui_Settings;

    //elements of the applet
    QList<ArtistWidget*> m_artists;

    //list of the separators
    QList<QWidget *> m_layoutWidgetList;

    /**
     * The max number artists
     */
    int m_maxArtists;
    /**
     * Artist which you want to see artists like
     */
    int m_temp_maxArtists;

    QString m_currentArtist;

private slots:

    /**
     * Allows the connection to the lastfm's api
     */
    void connectSource( const QString &source );

    /**
     * Show the settings windows
     */
    void configure();
    void switchToLang(QString lang);
    void changeMaxArtists( int value );
    void saveMaxArtists();
    void saveSettings();

};

K_EXPORT_AMAROK_APPLET( similarArtists, SimilarArtistsApplet )

#endif // SIMILARARTISTSAPPLET_H
