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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include <QObject>


// Keep header Objective-C++ clean! Pure C++ here. Communication to CocoaWatcher
// class please via befriended classes/functions - makes code more readable...

class CocoaWatcher : public QObject
{
	Q_OBJECT
	
	// Friends
	friend void setiPodMountState( CocoaWatcher*, bool, QString );
	friend void emitiTunesLaunched( CocoaWatcher* );
	
	public:
        CocoaWatcher();
		virtual ~CocoaWatcher();
        
        static CocoaWatcher* getInstance();
        static bool instanceFlag;
        static CocoaWatcher* single;
        
        void startCocoaMessagePump();
        void stopCocoaMessagePump();
		
		void watchOutForiPod();
		void watchOutForiTunes();
		
		void stopWatchers();
		
		bool iPodMounted() { return m_iPodMountState; };
		QString iPodPath() { return m_iPodPath; };
		
	private:
		QString m_iPodPath;
		bool m_iPodMountState;
        bool m_messagePumpRunning;

	signals:
		void iPodMounted( QString );
		void iPodUnMounted();
		void iPodMountStateChanged( bool, QString );
		void iTunesLaunched();
};
