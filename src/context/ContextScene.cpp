/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
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

#include "ContextScene.h"

#include "core/support/Amarok.h"
#include "amarokconfig.h"
#include "core/support/Debug.h"

#include <KStandardDirs>
#include <plasma/containment.h>
#include <plasma/theme.h>

#include <QGraphicsSceneDragDropEvent>


namespace Context
{

ContextScene::ContextScene( QObject * parent )
    : Plasma::Corona( parent )
{
    setBackgroundBrush( Qt::NoBrush );
}

ContextScene::~ContextScene()
{
    DEBUG_BLOCK
}

void ContextScene::loadDefaultSetup()
{
    // WORKAROUND for a bug in KDE 4.5.0 and 4.5.1:
    // Delete amarok-appletsrc config file (created by Plasma), because Plasma tries
    // to load all applets listed in there, which can lead to crashes due to applets
    // being loaded twice.
    // See: BUG 246756
    if( ( KDE::versionMajor() == 4 && KDE::versionMinor() == 5 && KDE::versionRelease() == 0 ) ||
        ( KDE::versionMajor() == 4 && KDE::versionMinor() == 5 && KDE::versionRelease() == 1 ) )
    {
        QFile::remove( KStandardDirs::locateLocal( "config", "amarok-appletsrc", false ) );
    }

    Plasma::Containment* c = addContainment( "amarok_containment_vertical" );
    c->setScreen( -1 );
    c->setFormFactor( Plasma::Planar );
}

} // Context namespace

#include "ContextScene.moc"

