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

#ifndef METADATACONFIG_H
#define METADATACONFIG_H

#include "ui_MetadataConfig.h"
#include "configdialog/ConfigDialogBase.h"

namespace StatSyncing {
    class Config;
}

class MetadataConfig : public ConfigDialogBase, private Ui_MetadataConfig
{
    Q_OBJECT

    public:
        MetadataConfig( QWidget *parent );
        virtual ~MetadataConfig();

        virtual bool isDefault();
        virtual bool hasChanged();
        virtual void updateSettings();

    Q_SIGNALS:
        void changed();

    private Q_SLOTS:
        void slotForgetCollections();
        void slotUpdateForgetButton();
        void slotUpdateConfigureExcludedLabelsLabel();
        void slotConfigureExcludedLabels();
        void slotConfigureProvider();
        void slotUpdateProviderConfigureButton();
        void slotCreateProviderDialog();

    private:
        int writeBackCoverDimensions() const;
        qint64 checkedFields() const;

        QWeakPointer<StatSyncing::Config> m_statSyncingConfig;

};

#endif // METADATACONFIG_H
