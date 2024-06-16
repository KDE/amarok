/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "ContextUrlRunner.h"

#include "MainWindow.h"
#include "AmarokUrlHandler.h"
#include "context/ContextView.h"
#include "context/AppletModel.h"

#include <KLocalizedString>

ContextUrlRunner::ContextUrlRunner()
{}

ContextUrlRunner::~ContextUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner ( this );
}

QIcon ContextUrlRunner::icon() const
{
    return QIcon::fromTheme( QStringLiteral("x-media-podcast-amarok") );
}

bool ContextUrlRunner::run( const AmarokUrl &url )
{
    DEBUG_BLOCK
    
    if( url.isNull() )
        return false;

    if( url.command() != command() )
        return false;


    QString appletsString = url.args().value( QStringLiteral("applets") );
    debug() << "applet string: " << appletsString;
    QStringList appletList = appletsString.split( QLatin1Char(',') );
    auto model = Context::ContextView::self()->appletModel();
    if( model )
    {
        model->clear();
        for( const QString &appletPluginName : appletList )
        {
            model->setAppletEnabled( appletPluginName, true );
        }
    }
    The::mainWindow()->showDock( MainWindow::AmarokDockContext );

    return true;
}

QString ContextUrlRunner::command() const
{
    return QStringLiteral("context");
}

QString ContextUrlRunner::prettyCommand() const
{
    return i18nc( "A type of command that affects the context view", "Context" );
}

