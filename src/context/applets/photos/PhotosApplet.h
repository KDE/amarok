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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

//Plasma applet for showing photos from flickr

#ifndef PHOTOS_APPLET_H
#define PHOTOS_APPLET_H

#include "context/Applet.h"
#include "context/DataEngine.h"

#include <ui_photosSettings.h>

class KConfigDialog;
class PhotosScrollWidget;
class QGraphicsSimpleTextItem;

namespace Plasma
{
    class IconWidget;
}

 /** PhotosApplet will display photos from the Internet, relative to the current playing song
   */
class PhotosApplet : public Context::Applet
{
    Q_OBJECT

    public:
        PhotosApplet( QObject* parent, const QVariantList& args );
        ~PhotosApplet();

    public Q_SLOTS:
        virtual void init();
        void dataUpdated( const QString& name, const Plasma::DataEngine::Data& data );
        void saveSettings();

    protected Q_SLOTS:
        void stopped();
        
    protected:
        void createConfigurationInterface(KConfigDialog *parent);

    private Q_SLOTS:
        void photoAdded();
        
    private:
        PhotosScrollWidget *m_widget;

        int m_nbPhotos;
        
        QString m_currentArtist;
        QString m_Animation;
        QStringList m_KeyWords;

        Ui::photosSettings      ui_Settings;
        Plasma::IconWidget      *m_settingsIcon;
};

AMAROK_EXPORT_APPLET( photos, PhotosApplet )

#endif /* Photos_APPLET_H */
