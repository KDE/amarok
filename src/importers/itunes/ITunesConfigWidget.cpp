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

#include "ITunesConfigWidget.h"

using namespace StatSyncing;

ITunesConfigWidget::ITunesConfigWidget( const QVariantMap &config, QWidget *parent,
                                        Qt::WindowFlags f )
    : ProviderConfigWidget( parent, f )
    , m_config( config )
{
    setupUi( this );

    m_targetName->setText( m_config.value( "name", "iTunes" ).toString() );
    m_databaseLocation->setText( m_config.value( "dbPath", "" ).toString() );
}

ITunesConfigWidget::~ITunesConfigWidget()
{
}

QVariantMap
ITunesConfigWidget::config() const
{
    QVariantMap cfg = m_config;
    cfg.insert( "name", m_targetName->text() );
    cfg.insert( "dbPath", m_databaseLocation->text() );
    return cfg;
}
