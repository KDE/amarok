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

#ifndef STATSYNCING_FAST_FORWARD_CONFIG_WIDGET_H
#define STATSYNCING_FAST_FORWARD_CONFIG_WIDGET_H

#include "statsyncing/Provider.h"
#include "ui_FastForwardConfigWidget.h"

namespace StatSyncing
{

class FastForwardConfigWidget : public ProviderConfigWidget,
        public Ui::FastForwardConfigWidget
{
    Q_OBJECT

public:
    explicit FastForwardConfigWidget( const QVariantMap &config, QWidget *parent = nullptr,
                                      Qt::WindowFlags f = 0 );
    ~FastForwardConfigWidget();

    QVariantMap config() const;

    enum Driver
    {
        QMYSQL,
        QPSQL,
        QSQLITE
    };
    Q_ENUM( Driver )

private:
    void populateFields();

    const QVariantMap m_config;
    QList<QWidget*> m_externalDbSettings;
    QList<QWidget*> m_embeddedDbSettings;

private Q_SLOTS:
    void connectionTypeChanged( const int index );
};

} // namespace StatSyncing

#endif // STATSYNCING_FAST_FORWARD_CONFIG_WIDGET_H
