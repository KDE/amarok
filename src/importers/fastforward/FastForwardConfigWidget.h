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

#ifndef STATSYNCING_FAST_FORWARD_CONFIG_H
#define STATSYNCING_FAST_FORWARD_CONFIG_H

#include "ui_FastForwardConfigWidget.h"
#include "statsyncing/Provider.h"

#include <QObject>
#include <QString>

namespace StatSyncing
{

class FastForwardConfigWidget : public ProviderConfigWidget,
        public Ui::FastForwardConfigWidget
{
    Q_OBJECT

public:
    FastForwardConfigWidget( const QVariantMap &config, QWidget *parent = 0,
                             Qt::WindowFlags f = 0 );
    virtual ~FastForwardConfigWidget();

    QVariantMap config() const;

public slots:
    void connectionTypeChanged( QString connection );

private:
    void populateFields();

    QVariantMap m_config;
};

} // namespace StatSyncing

#endif // STATSYNCING_FAST_FORWARD_CONFIG_H
