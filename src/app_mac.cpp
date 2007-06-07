#include <Carbon/Carbon.h>
#include <QStringList>
#include "dynamicmode.h"
#include "playlistwindow.h"

static AEEventHandlerUPP appleEventProcessorUPP = 0;

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
            if( PlaylistWindow::self() )
                PlaylistWindow::self()->show();
        }
        return noErr;
    }
    return eventNotHandledErr;
}

void
setupEventHandler_mac(long handlerRef)
{
    appleEventProcessorUPP = AEEventHandlerUPP(appleEventProcessor);
    AEInstallEventHandler(kCoreEventClass, kAEReopenApplication, appleEventProcessorUPP, handlerRef, true);
}
