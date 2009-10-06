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

#include "ContextUrlRunner.h"

#include "AmarokUrlHandler.h"
#include "context/ContextView.h"

ContextUrlRunner::ContextUrlRunner()
{}

ContextUrlRunner::~ContextUrlRunner()
{
    The::amarokUrlHandler()->unRegisterRunner ( this );
}

KIcon ContextUrlRunner::icon() const
{
    return KIcon( "x-media-podcast-amarok" );
}

bool ContextUrlRunner::run( AmarokUrl url )
{
    DEBUG_BLOCK
    
    if( url.isNull() )
        return false;

    if( url.command() != command() )
        return false;


    QString appletsString = url.args().value( "applets" );
    debug() << "applet string: " << appletsString;
    QStringList appletList = appletsString.split( "," );

    Context::ContextView::self()->clearNoSave();
    Context::Containment* cont = dynamic_cast< Context::Containment* >( Context::ContextView::self()->containment() );
    if( cont )
    {
        foreach( QString appletPluginName, appletList )
        {
            cont->addApplet( appletPluginName, -1 );
        }
    }

}

QString ContextUrlRunner::command() const
{
    return "context";
}

