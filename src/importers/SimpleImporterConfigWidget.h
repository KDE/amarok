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

#ifndef STATSYNCING_SIMPLE_IMPORTER_CONFIG_WIDGET
#define STATSYNCING_SIMPLE_IMPORTER_CONFIG_WIDGET

#include "statsyncing/Provider.h"

#include <QMap>

class QGridLayout;

namespace StatSyncing
{

class AMAROK_EXPORT SimpleImporterConfigWidget : public ProviderConfigWidget
{
public:
    SimpleImporterConfigWidget( const QString &targetName, const QVariantMap &config,
                                QWidget *parent = 0, Qt::WindowFlags f = 0 );
    ~SimpleImporterConfigWidget();

    void addField( const QString &configName, const QString &label,
                   QWidget * const field, const QString &property );

    QVariantMap config() const;

private:
    const QVariantMap m_config;
    QMap<QString, QPair<QWidget*, QString> > m_fieldForName;
    QGridLayout *m_layout;
};

} // namespace StatSyncing

#endif // STATSYNCING_SIMPLE_IMPORTER_CONFIG_WIDGET
