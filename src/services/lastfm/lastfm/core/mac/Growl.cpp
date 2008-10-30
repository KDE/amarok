/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
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

#include "Growl.h"
#include "AppleScript.h"
#include "../CoreProcess.h"
#include <QCoreApplication>
#include <QFileInfo>


Growl::Growl( const QString& name )
     : m_name( name )
{}


void
Growl::notify()
{
    if (!CoreProcess::isRunning( "GrowlHelperApp" ))
        return;

    AppleScript script;
    script << "tell application 'GrowlHelperApp'"
           <<     "register as application '" + qApp->applicationName() + "'"
                          " all notifications {'" + m_name + "'}"
                          " default notifications {'" + m_name + "'}"
                          " icon of application 'Last.fm.app'"
           <<     "notify with name '" + m_name + "'"
                          " title " + AppleScript::asUnicodeText( m_title ) +
                          " description " + AppleScript::asUnicodeText( m_description ) + 
                          " application name '" + qApp->applicationName() + "'"
           << "end tell";
    script.exec();
}
