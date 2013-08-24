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

#ifndef STATSYNCING_BANSHEE_CONFIG_WIDGET_H
#define STATSYNCING_BANSHEE_CONFIG_WIDGET_H

#include "statsyncing/Provider.h"
#include "ui_BansheeConfigWidget.h"

namespace StatSyncing
{

class BansheeConfigWidget : public ProviderConfigWidget, public Ui::BansheeConfigWidget
{
public:
    BansheeConfigWidget( const QVariantMap &config, QWidget *parent = 0,
                         Qt::WindowFlags f = 0 );
    ~BansheeConfigWidget();

    QVariantMap config() const;

private:
    const QVariantMap m_config;
};

} // namespace StatSyncing

#endif // STATSYNCING_BANSHEE_CONFIG_WIDGET_H
