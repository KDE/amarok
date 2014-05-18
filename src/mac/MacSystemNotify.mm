/****************************************************************************************
 * Copyright (c) 2014 Daniel Meltzer <parallelgrapefruit@gmail.com                      *
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

#include "MacSystemNotify.h"

#include "amarokconfig.h"
#include "App.h"
#include "CoverManager/CoverCache.h"
#include "core/support/Debug.h"
#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaUtility.h" // for secToPrettyTime
#include "SvgHandler.h"
#include "TrayIcon.h"

#import <Foundation/NSUserNotification.h>
#import <ApplicationServices/ApplicationServices.h>

namespace {
    void SendNotifactionCenterMessage(NSString* title, NSString* subtitle, NSImage *image) {
        NSUserNotificationCenter* center =
            [NSUserNotificationCenter defaultUserNotificationCenter];
        NSUserNotification *notification =
            [[NSUserNotification alloc] init];

        [notification setTitle: title];
        [notification setSubtitle: subtitle];
        [notification setContentImage: image];

        [center deliverNotification: notification];

        [notification release];
    }
}

OSXNotify::OSXNotify(QString appName): QObject()
               , m_appName( appName )
{
    EngineController *engine = The::engineController();

    connect( engine, SIGNAL(trackChanged(Meta::TrackPtr)),
             this, SLOT(show(Meta::TrackPtr)) );
}

void
OSXNotify::show( Meta::TrackPtr track )
{
    DEBUG_BLOCK
    QString text;
    if( !track || track->playableUrl().isEmpty() )
        text = i18n( "No track playing" );
    else
    {
        text = track->prettyName();
        if( track->artist() && !track->artist()->prettyName().isEmpty() )
            text = track->artist()->prettyName() + " - " + text;
        if( track->album() && !track->album()->prettyName().isEmpty() )
            text += "\n (" + track->album()->prettyName() + ") ";
        else
            text += '\n';
        if( track->length() > 0 )
            text += Meta::msToPrettyTime( track->length() );
    }

    if( text.isEmpty() )
        text =  track->playableUrl().fileName();

    if( text.startsWith( "- " ) ) //When we only have a title tag, _something_ prepends a fucking hyphen. Remove that.
        text = text.mid( 2 );

    if( text.isEmpty() ) //still
        text = i18n("No information available for this track");

    QPixmap pixmap = The::coverCache()->getCover( track->album(), 100 );
    CGImageRef cgImage = pixmap.toMacCGImageRef();
    NSImage *nImage = [[NSImage alloc] initWithCGImage:cgImage size:NSZeroSize];
    CFRelease(cgImage);
    NSString *title =[[NSString alloc] initWithUTF8String: "Amarok" ];
    NSString *subTitle = [[NSString alloc] initWithUTF8String:text.toUtf8().constData() ];
    SendNotifactionCenterMessage( title, subTitle, nImage );
}
