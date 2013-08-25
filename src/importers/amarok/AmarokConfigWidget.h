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

#ifndef STATSYNCING_AMAROK_CONFIG_WIDGET_H
#define STATSYNCING_AMAROK_CONFIG_WIDGET_H

#include "statsyncing/Provider.h"
#include "ui_AmarokConfigWidget.h"

namespace StatSyncing
{

class AmarokConfigWidget : public ProviderConfigWidget, public Ui::AmarokConfigWidget
{
    Q_OBJECT

public:
    explicit AmarokConfigWidget( const QVariantMap &config, QWidget *parent = 0,
                                 Qt::WindowFlags f = 0 );
    ~AmarokConfigWidget();

    QVariantMap config() const;

private:
    enum ConnectionType
    {
        Embedded,
        External
    };

    void populateFields();

    const QVariantMap m_config;
    QList<QWidget*> m_externalDbSettings;
    QList<QWidget*> m_embeddedDbSettings;

private slots:
    void connectionTypeChanged( const int index );
};

} // namespace StatSyncing

#endif // STATSYNCING_AMAROK_CONFIG_WIDGET_H
