/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "BansheeConfigWidget.h"

using namespace StatSyncing;

BansheeConfigWidget::BansheeConfigWidget( const QVariantMap &config, QWidget *parent,
                                          Qt::WindowFlags f )
    : ProviderConfigWidget( parent, f )
    , m_config( config )
{
    setupUi( this );

    m_targetName->setText( m_config.value( "name", "Banshee" ).toString() );

    const QString defaultPath = QDir::toNativeSeparators(
                QDir::homePath() + "/.config/banshee-1/banshee.db" );
    m_databaseLocation->setText( m_config.value( "dbPath", defaultPath ).toString() );
}

BansheeConfigWidget::~BansheeConfigWidget()
{
}

QVariantMap
BansheeConfigWidget::config() const
{
    QVariantMap cfg( m_config );

    cfg.insert( "name", m_targetName->text() );
    cfg.insert( "dbPath", m_databaseLocation->text() );

    return cfg;
}
