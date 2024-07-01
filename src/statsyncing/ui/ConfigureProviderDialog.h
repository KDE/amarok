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

#ifndef STATSYNCING_CONFIGURE_PROVIDER_DIALOG_H
#define STATSYNCING_CONFIGURE_PROVIDER_DIALOG_H

#include <KPageDialog>

#include <QMap>
#include <QSharedPointer>
#include <QString>

namespace StatSyncing
{


    class ConfigureProviderDialog : public KPageDialog
    {
        Q_OBJECT

    public:
        explicit ConfigureProviderDialog( const QString &providerId, QWidget *configWidget,
                                          QWidget *parent = nullptr, Qt::WindowFlags f = {} );
        ~ConfigureProviderDialog() override;

    Q_SIGNALS:
        void providerConfigured( const QString &id, const QVariantMap &config );

    private:
        QString m_providerId;
        QWidget *mainWidget;

    private Q_SLOTS:
        void slotAccepted();
    };

} // namespace StatSyncing

#endif // STATSYNCING_CONFIGURE_PROVIDER_DIALOG_H
