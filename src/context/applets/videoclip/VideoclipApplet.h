/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

//Plasma applet for showing video from youtube dailymotion and vimeo in the CV

#ifndef VIDEOCLIP_APPLET_H
#define VIDEOCLIP_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "EngineObserver.h"

#include "../../engines/videoclip/VideoclipInfo.h"

#include <ui_videoclipSettings.h>

#include <Phonon/Path>
#include <Phonon/MediaObject>

// forward
namespace Phonon
{
    class MediaObject;
    class Path;
    class VideoWidget;
}

namespace Plasma
{
    class IconWidget;
}

class KConfigDialog;
class KratingWidget;
class KratingPainter;
class QAction;
class QGraphicsLinearLayout;
class QGraphicsProxyWidget;
class QGraphicsWidget;
class QHBoxLayout;

class CustomVideoWidget;


 /** VideoclipApplet will display videoclip from internet, relatively to the current playing song
   * If a video is detected in the playlist, it will also play trhe video inside the a VideoWidget.
   */
class VideoclipApplet : public Context::Applet, public EngineObserver
{
        Q_OBJECT

    public:
        VideoclipApplet( QObject* parent, const QVariantList& args );
        ~VideoclipApplet();

        void    init();
        void    paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

        void    constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );
        QSizeF  sizeHint( Qt::SizeHint which, const QSizeF & constraint = QSizeF() ) const;

        // inherited from EngineObserver
        virtual void engineNewTrackPlaying();
        virtual void enginePlaybackEnded( int finalPosition, int trackLength, PlaybackEndedReason reason );


    public slots:
        void    dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );
        void    connectSource( const QString &source );

        // right click context menu
        void    appendVideoClip( VideoInfo *info );
        void    queueVideoClip( VideoInfo *info );
        void    appendPlayVideoClip( VideoInfo *info );

        void    videoMenu( QPoint );
        void    saveSettings();
    protected:
        void    createConfigurationInterface(KConfigDialog *parent);


    private:
        Plasma::IconWidget * addAction( QAction *action );
        
        Phonon::MediaObject *m_mediaObject;
     //   Phonon::VideoWidget *m_videoWidget;
     CustomVideoWidget  *m_videoWidget;
        Phonon::Path        m_path;

        // The two big container, only one who need a resize
        QGraphicsSimpleTextItem *m_headerText;
        QGraphicsProxyWidget    *m_widget;
        QHBoxLayout             *m_layout;
        QList<QWidget *>m_layoutWidgetList;

        int m_height;
        QPixmap     *m_pixYoutube;
        QPixmap     *m_pixDailymotion;
        QPixmap     *m_pixVimeo;

        Plasma::IconWidget *m_settingsIcon;
        bool        m_youtubeHQ;

        Ui::videoclipSettings ui_Settings;
};

#endif /* VIDEOCLIP_APPLET_H */
