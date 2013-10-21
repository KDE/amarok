/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "Provider.h"

#include <QCoreApplication>

using namespace StatSyncing;

ProviderConfigWidget::ProviderConfigWidget( QWidget *parent, Qt::WindowFlags f )
    : QWidget( parent, f )
{
}

ProviderConfigWidget::~ProviderConfigWidget()
{
}

Provider::Provider()
{
    // ensure this object is created in a main thread
    Q_ASSERT( thread() == QCoreApplication::instance()->thread() );
}

Provider::~Provider()
{
}

QString
Provider::description() const
{
    return QString();
}

bool
Provider::isConfigurable() const
{
    return false;
}

ProviderConfigWidget*
Provider::configWidget()
{
    return 0;
}

void
Provider::reconfigure( const QVariantMap &config )
{
    Q_UNUSED( config )
}

void
Provider::commitTracks()
{
}
