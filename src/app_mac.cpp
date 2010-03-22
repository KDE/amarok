/****************************************************************************************
 * Copyright (c) 2007 Benjamin Reed <ranger@befunk.com>                                 *
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

#include <Carbon/Carbon.h>

#include "amarokurls/AmarokUrl.h"
#include "CollectionManager.h"
#include "core/support/Debug.h"
#include "DirectoryLoader.h"
#include "core/meta/Meta.h"
#include "core/playlists/Playlist.h"
#include "core-implementations/playlists/file/PlaylistFileSupport.h"
#include "playlist/PlaylistController.h"



#include <QByteArray>

#include <KUrl>

static AEEventHandlerUPP appleEventProcessorUPP = 0;
static AEEventHandlerUPP macCallbackUrlHandlerUPP = 0;

OSStatus
appleEventProcessor(const AppleEvent *ae, AppleEvent *, long /*handlerRefCon*/)
{
    OSType aeID = typeWildCard;
    OSType aeClass = typeWildCard;
    AEGetAttributePtr(ae, keyEventClassAttr, typeType, 0, &aeClass, sizeof(aeClass), 0);
    AEGetAttributePtr(ae, keyEventIDAttr, typeType, 0, &aeID, sizeof(aeID), 0);

    if(aeClass == kCoreEventClass)
    {
        if(aeID == kAEReopenApplication)
        {
#if 0
            if( PlaylistWindow::self() )
                PlaylistWindow::self()->show();
#endif
        }
        return noErr;
    }
    return eventNotHandledErr;
}

OSStatus
macCallbackUrlHandler( const AppleEvent *ae, AppleEvent *, long /*handlerRefCon*/)
{
    DEBUG_BLOCK
    OSErr error = noErr;
    Size actualSize = 0;
    DescType descType = typeUTF8Text;
    if( ( error = AESizeOfParam( ae, keyDirectObject, &descType, &actualSize ) ) == noErr )
    {
        QByteArray ba;
        ba.resize( actualSize + 1 );
        error = AEGetParamPtr( ae, keyDirectObject, typeUTF8Text, 0, ba.data(), actualSize, &actualSize );
        if( error == noErr )
        {
            KUrl url( QString::fromUtf8( ba.data() ) );
            if( url.protocol() == "amarok" )
            {
                AmarokUrl aUrl( url.url() );
                aUrl.run();
            } else
            {
                DirectoryLoader* loader = new DirectoryLoader();
                QList<KUrl> urls;
                urls << url;
                loader->init(urls);
            }
        }
    }
    return error;
}

void
setupEventHandler_mac(long handlerRef)
{
    appleEventProcessorUPP = AEEventHandlerUPP(appleEventProcessor);
    AEInstallEventHandler(kCoreEventClass, kAEReopenApplication, appleEventProcessorUPP, handlerRef, true);
    macCallbackUrlHandlerUPP = AEEventHandlerUPP(macCallbackUrlHandler);
    AEInstallEventHandler(kInternetEventClass, kAEGetURL, macCallbackUrlHandlerUPP, handlerRef, false);
}

