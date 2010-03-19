/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
 * Copyright (c) 2010 Emmanuel Wagner <manu.wagner@sfr.fr>                              *
 * Copyright (c) 2010 Mark Kretschmann <kretschmann@kde.org>                            *
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

//Plasma applet for showing photos from flickr

#ifndef COVERBLING_APPLET_H
#define COVERBLING_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "EngineObserver.h"
#include "PhotoBrowser.h"
#include "ui_coverblingSettings.h"

class TextScrollingWidget;
class KConfigDialog;
class PhotosScrollWidget;
class QGraphicsSimpleTextItem;
class QGraphicsProxyWidget;
class RatingWidget;
class QGraphicsPixmapItem;

namespace Plasma
{
    class IconWidget;
}

class CoverBlingApplet : public Context::Applet, public EngineObserver
{
    Q_OBJECT

    public:
        CoverBlingApplet( QObject* parent, const QVariantList& args );
        ~CoverBlingApplet();

        void init();
        void paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );
                
	// inherited from EngineObserver
        virtual void engineNewTrackPlaying();

    public slots:
        void slotAlbumQueryResult( QString collectionId, Meta::AlbumList albums);
        void slideChanged( int islideindex );
        void appendAlbum( int islideindex );
        void toggleFullscreen();
        void jumpToPlaying();
        void saveSettings();
        void skipToFirst();
        void skipToLast();

    protected :
        virtual void constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );
        void createConfigurationInterface(KConfigDialog *parent);

    private:
        PhotoBrowser * m_pictureflow;
        QGraphicsProxyWidget * m_layout;
        RatingWidget* m_ratingWidget;
        QGraphicsSimpleTextItem* m_label;

        Plasma::IconWidget* m_blingtofirst;
        Plasma::IconWidget* m_blingtolast;
        Plasma::IconWidget* m_blingfastback;
        Plasma::IconWidget* m_blingfastforward;
        Plasma::IconWidget* m_fullscreen;
        Plasma::IconWidget* m_jumptoplaying;

        bool m_fullsize;
        bool m_autojump;
        bool m_animatejump;
        Ui::coverblingSettings   ui_Settings;
        int m_coversize;
        PictureFlow::ReflectionEffect m_reflectionEffect;
        bool m_openGL;
};

K_EXPORT_AMAROK_APPLET( coverbling, CoverBlingApplet )

#endif /* COVERBLING_APPLET_H */
