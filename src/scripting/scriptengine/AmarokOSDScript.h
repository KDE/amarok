/****************************************************************************************
 * Copyright (c) 2008 Peter ZHOU <peterzhoulei@gmail.com>                               *
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

#ifndef AMAROK_OSD_SCRIPT_H
#define AMAROK_OSD_SCRIPT_H

#include <QObject>

class QColor;
class QImage;
class QJSEngine;

namespace AmarokScript
{
    // SCRIPTDOX: Amarok.Window.OSD
    class AmarokOSDScript : public QObject
    {
        Q_OBJECT

        Q_PROPERTY ( bool osdEnabled READ osdEnabled WRITE setOsdEnabled )

        public:
            explicit AmarokOSDScript( QJSEngine* scriptEngine );

            /**
             * Show an OSD for the currently playing track, even if the OSD
             * has been disabled by the user.
             */
            Q_INVOKABLE void showCurrentTrack();

            /**
             * Forces an OSD update.
             * OSD settings changes do not take effect until the next time
             * the OSD is shown.
             * To show an OSD with the current settings, call show();
             */
            Q_INVOKABLE void show();

            /**
             * Set the OSD duration
             */
            Q_INVOKABLE void setDuration( int ms );

            /**
             * Set the OSD textcolor
             */
            Q_INVOKABLE void setTextColor( const QColor &color );

            /**
             * Set the OSD's y-offset.
             */
            Q_INVOKABLE void setOffset( int y );

            /**
             * Set the image to be shown in the OSD.
             */
            Q_INVOKABLE void setImage( const QPixmap &image );

            /**
             * Set the screen on which to show the OSD.
             */
            Q_INVOKABLE void setScreen( int screen );

            /**
             * Set the OSD text
             */
            Q_INVOKABLE void setText( const QString &text );

            /**
             * Set the number of half-stars to be shown in the OSD.
             * Amarok must be playing a track for the stars to show.
             */
            Q_INVOKABLE void setRating( const short rating );

        private:
            void setOsdEnabled( bool enable );
            bool osdEnabled();
    };
}

#endif
