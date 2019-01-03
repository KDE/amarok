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

#ifndef STATSYNCING_CREATE_PROVIDER_DIALOG_H
#define STATSYNCING_CREATE_PROVIDER_DIALOG_H

#include <KAssistantDialog>

#include <QButtonGroup>
#include <QMap>
#include <QSharedPointer>
#include <QString>

#include <QIcon>

class QVBoxLayout;

namespace StatSyncing
{

    class ProviderConfigWidget;

    class CreateProviderDialog : public KAssistantDialog
    {
        Q_OBJECT

    public:
        explicit CreateProviderDialog( QWidget *parent = nullptr, Qt::WindowFlags f = 0 );
        virtual ~CreateProviderDialog();

    public Q_SLOTS:
        void addProviderType( const QString &id, const QString &prettyName, const QIcon &icon,
                              ProviderConfigWidget *configWidget );

    Q_SIGNALS:
        void providerConfigured( const QString &id, const QVariantMap &config );

    private:
        int buttonInsertPosition( const QString &prettyName );

        QButtonGroup m_providerButtons;
        QMap<QObject*, QString> m_idForButton;
        QMap<QObject*, KPageWidgetItem*> m_configForButton;
        KPageWidgetItem *m_providerTypePage;
        QVBoxLayout *m_layout;

    private Q_SLOTS:
        void providerButtonToggled( bool checked );
        void slotAccepted();
    };

} // namespace StatSyncing

#endif // STATSYNCING_CREATE_PROVIDER_DIALOG_H
