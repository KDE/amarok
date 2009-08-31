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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

//Plasma applet for showing photos from flickr

#ifndef PHOTOS_APPLET_H
#define PHOTOS_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"
#include "EngineObserver.h"

#include "../../engines/photos/PhotosInfo.h"

#include <ui_photosSettings.h>

class TextScrollingWidget;
class KConfigDialog;
class PhotosScrollWidget;
class QGraphicsSimpleTextItem;

namespace Plasma
{
    class IconWidget;
}

 /** PhotosApplet will display photos from internet, relatively to the current playing song
   */
class PhotosApplet : public Context::Applet, public EngineObserver
{
        Q_OBJECT

    public:
        PhotosApplet( QObject* parent, const QVariantList& args );
        ~PhotosApplet();

        void    init();
        void    paintInterface( QPainter *painter, const QStyleOptionGraphicsItem *option, const QRect &contentsRect );

        void    constraintsEvent( Plasma::Constraints constraints = Plasma::AllConstraints );

        // inherited from EngineObserver
        virtual void engineNewTrackPlaying();
        virtual void enginePlaybackEnded( int finalPosition, int trackLength, PlaybackEndedReason reason );
        
    public slots:
        void    dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );
        void    connectSource( const QString &source );
        void    saveSettings();

    protected:
        void createConfigurationInterface(KConfigDialog *parent);
        
    private:
        TextScrollingWidget     *m_headerText;
        PhotosScrollWidget      *m_widget;

        int   m_height;
        int   m_nbPhotos;
        bool  m_stoppedstate;
        
        QString m_Animation;
        QString m_KeyWords;

        Ui::photosSettings      ui_Settings;
        Plasma::IconWidget      *m_settingsIcon;
};

Q_DECLARE_METATYPE ( QList < PhotosInfo * > )
K_EXPORT_AMAROK_APPLET( photos, PhotosApplet )

#endif /* Photos_APPLET_H */
