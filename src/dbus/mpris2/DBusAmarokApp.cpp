/***********************************************************************
 * Copyright 2010  Canonical Ltd
 *   (author: Aurelien Gateau <aurelien.gateau@canonical.com>)
 * Copyright 2012  Alex Merry <alex.merry@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************/

#include "DBusAmarokApp.h"

#include "SvgHandler.h"
#include "core/support/Debug.h"
#include "widgets/Osd.h"

using namespace Amarok;

DBusAmarokApp::DBusAmarokApp(QObject* parent)
    : DBusAbstractAdaptor(parent)
{
}

DBusAmarokApp::~DBusAmarokApp()
{
}

void DBusAmarokApp::ShowOSD() const
{
    Amarok::OSD::instance()->forceToggleOSD();
}

void DBusAmarokApp::LoadThemeFile( const QString &path )
{
    The::svgHandler()->setThemeFile( path );
}

