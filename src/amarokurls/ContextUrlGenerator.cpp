/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ContextUrlGenerator.h"

#include "AmarokUrl.h"
#include "context/ContextView.h"

#include <KLocale>

ContextUrlGenerator::ContextUrlGenerator()
{
}

ContextUrlGenerator::~ContextUrlGenerator()
{
}

AmarokUrl
ContextUrlGenerator::createContextBookmark()
{
    QStringList pluginNames = Context::ContextView::self()->currentApplets();
    QStringList appletNames = Context::ContextView::self()->currentAppletNames();

    AmarokUrl url;

    url.setCommand( "context" );
    url.appendArg( "applets", pluginNames.join( "," ) );

    url.setName( i18n( "Context: %1", appletNames.join( "," ) ) );

    return url;
}

#include "ContextUrlGenerator.h"

