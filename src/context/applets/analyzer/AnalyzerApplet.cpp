/****************************************************************************************
 * Copyright (c) 2013 Mark Kretschmann <kretschmann@kde.org>                            *
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

#define DEBUG_PREFIX "AnalyzerApplet"

#include "AnalyzerApplet.h"

#include "App.h"
#include "analyzer/BlockAnalyzer.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <QGraphicsLinearLayout>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>


AnalyzerApplet::AnalyzerApplet( QObject* parent, const QVariantList& args )
    : Context::Applet( parent, args )
{
    setHasConfigurationInterface( false );
}

AnalyzerApplet::~AnalyzerApplet()
{}

void
AnalyzerApplet::init()
{
    // Call the base implementation.
    Context::Applet::init();

    BlockAnalyzer *analyzer = new BlockAnalyzer( 0 );

    QGraphicsProxyWidget *proxy = scene()->addWidget( analyzer );

    QGraphicsLinearLayout *layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    layout->addItem( proxy );

    updateConstraints();
}

#include "AnalyzerApplet.moc"
