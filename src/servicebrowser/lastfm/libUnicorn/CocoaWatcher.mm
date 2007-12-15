/***************************************************************************
 *   Copyright (C) 2007 by                                		           *
 *      Philipp Maihart, Last.fm Ltd <phil@last.fm>      		           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#import <Cocoa/Cocoa.h>
#import "CocoaWatcher.h"


// Friend functions
void setiPodMountState( CocoaWatcher* cocoa, bool mounted, QString path )
{
	if ( mounted == true )
		emit cocoa->iPodMounted( path );	
	else
		emit cocoa->iPodUnMounted();
	
	cocoa->m_iPodMountState = mounted;
	cocoa->m_iPodPath = path;
	
	qDebug() << "iPod state changed: Mounted =" << mounted << "Path =" << path;
	emit cocoa->iPodMountStateChanged( mounted, path );
}

void emitiTunesLaunched( CocoaWatcher* cocoa )
{
	qDebug() << "iTunes launched";
	emit cocoa->iTunesLaunched();
}


// Objective-C Interfaces
@interface IPodWatcher : NSObject
{
	NSString *iPodMountPath;
	CocoaWatcher *cUtils;
}
- (id)init;
- (void)connect:(CocoaWatcher*)parent;
- (void)volumeDidMount:(NSNotification*)notification;
- (void)volumeDidUnmount:(NSNotification*)notification;
@end

@interface ITunesWatcher : NSObject
{
    CocoaWatcher *cUtils;
}
- (id)init;
- (void)connect:(CocoaWatcher*)parent;
- (void)appLaunched:(NSNotification*)notification;
@end


// Global variables
NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
IPodWatcher *iPodWatcher;
ITunesWatcher *iTunesWatcher;


// Constructor/Destructor
CocoaWatcher::CocoaWatcher() :
    m_messagePumpRunning( false )
{}

CocoaWatcher::~CocoaWatcher()
{
    stopWatchers();
	[pool release];
    instanceFlag = false;
}

bool CocoaWatcher::instanceFlag = false;
CocoaWatcher* CocoaWatcher::single = NULL;

CocoaWatcher* CocoaWatcher::getInstance()
{
    if( !instanceFlag )
    {
        single = new CocoaWatcher();
        instanceFlag = true;
        return single;
    }
    else
    {
        return single;
    }
}

// Some Notifications need a running Cocoa Message Pump - be careful!
// Only call if you need that for your notification...
void
CocoaWatcher::startCocoaMessagePump()
{
    if ( m_messagePumpRunning == true ) return;
    
    m_messagePumpRunning = true;

    [NSApp run];
}

void
CocoaWatcher::stopCocoaMessagePump()
{
    if ( m_messagePumpRunning == false ) return;

    [NSApp terminate];

    m_messagePumpRunning = false;
}


// Start watchers
void
CocoaWatcher::watchOutForiPod()
{
	iPodWatcher = [[IPodWatcher alloc] init];
	CocoaWatcher* me = this; // C++
	[iPodWatcher connect:me];
}


void
CocoaWatcher::watchOutForiTunes()
{
	iTunesWatcher = [[ITunesWatcher alloc] init];
	CocoaWatcher* me = this; // C++
	[iTunesWatcher connect:me];
}


// Stops all observing watchers
void
CocoaWatcher::stopWatchers()
{
    [iPodWatcher release];
	[iTunesWatcher release];
	stopCocoaMessagePump();
}


// Implementations for Obj-C classes

// Emits a signals when mounting/unmounting an iPod
@implementation IPodWatcher
- (id)init
{		
    // Register for mounts and unmounts (iPod support)
	[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
        selector:@selector(volumeDidMount:) name:NSWorkspaceDidMountNotification object:nil];
	[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
        selector:@selector(volumeDidUnmount:) name:NSWorkspaceDidUnmountNotification object:nil];

	return self;
}

- (void)connect:(CocoaWatcher*)parent
{
	cUtils = parent; // C++

	if (!iPodMountPath) {
        // Simulate mount events for current mounts so that any mounted iPod is found
        NSEnumerator *en = [[[NSWorkspace sharedWorkspace] mountedLocalVolumePaths] objectEnumerator];
        NSString *path;
        while ((path = [en nextObject])) {
            NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:path, @"NSDevicePath", nil];
            NSNotification *note = [NSNotification notificationWithName:NSWorkspaceDidMountNotification
                object:[NSWorkspace sharedWorkspace] userInfo:dict];
            [self volumeDidMount:note];
        }
    }	
}

- (void)volumeDidMount:(NSNotification*)notification
{
    NSDictionary *info = [notification userInfo];
	NSString *mountPath = [info objectForKey:@"NSDevicePath"];
    NSString *iPodControlPath = [mountPath stringByAppendingPathComponent:@"iPod_Control"];

    BOOL isDir = NO;
    if ([[NSFileManager defaultManager] fileExistsAtPath:iPodControlPath isDirectory:&isDir] && isDir) {
        [self setValue:mountPath forKey:@"iPodMountPath"];

		setiPodMountState( cUtils, true, QString::fromStdString( std::string( [mountPath cString] ) ) ); // C++
    }
}

- (void)volumeDidUnmount:(NSNotification*)notification
{
	if ( cUtils == NULL ) return;
	
    NSDictionary *info = [notification userInfo];
	NSString *mountPath = [info objectForKey:@"NSDevicePath"];
    
    if ([iPodMountPath isEqualToString:mountPath]) {
		setiPodMountState( cUtils, false, "" ); // C++
    }
}
@end


// Emits a signal when launching iTunes
@implementation ITunesWatcher
- (id)init
{
    // Register for application launches
	[[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
        selector:@selector(appLaunched:) name:NSWorkspaceDidLaunchApplicationNotification object:nil];
    
    return self;
}

- (void)connect:(CocoaWatcher*)parent
{
	cUtils = parent; // C++    
    cUtils->startCocoaMessagePump(); // Need message pump for this notification
}
 
- (void)appLaunched:(NSNotification*)notification
{
	if ( cUtils == NULL ) return;
    
    NSDictionary *info = [notification userInfo];

	if ( [[info objectForKey:@"NSApplicationName"] isEqual:@"iTunes"] )
	{
		emitiTunesLaunched( cUtils ); // C++
    }
}
@end
