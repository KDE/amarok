/****************************************************************************************
 * Copyright (c) 2008-2012 Soren Harward <stharward@gmail.com>                          *
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

#ifndef APG_PRESET_MODEL
#define APG_PRESET_MODEL

#include "Preset.h"

#include <QAbstractItemModel>
#include <QFileDialog>
#include <QList>
#include <QString>

class QPersistentModelIndex;

namespace APG {
    class PresetModel : public QAbstractListModel {
        Q_OBJECT

        public:
            static PresetModel* instance();
            static void destroy();
            static const QString presetExamples; // holds a hard-coded set of example presets
                                                 // that are loaded the first time the
                                                 // user opens the APG

            // Overloaded QAbstractListModel methods
            QVariant data( const QModelIndex&, int role = Qt::DisplayRole ) const override;
            QModelIndex index( int, int, const QModelIndex& parent = QModelIndex() ) const override;
            QModelIndex parent( const QModelIndex& ) const override { return QModelIndex(); }
            int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
            int columnCount( const QModelIndex& ) const override { return 1; }

            APG::PresetPtr activePreset() const;

        Q_SIGNALS:
            void lock( bool ); // disable the edit widgets if the solver is running

        public Q_SLOTS:
            void addNew();
            void edit();
            void editPreset( const QModelIndex& );
            void exportActive();
            void import();
            void removeActive();
            void runGenerator( int );
            void setActivePreset( const QModelIndex& );
            void savePresetsToXmlDefault() const; // force saving to default location

        private Q_SLOTS:
            void savePresetsToXml( const QString&, const QList<APG::PresetPtr> & ) const;
            void loadPresetsFromXml( const QString&, bool createDefaults = false );

        private:
            PresetModel();
            ~PresetModel() override;
            static PresetModel* s_instance;

            class ExportDialog;

            void insertPreset(const APG::PresetPtr&);
            void parseXmlToPresets( QDomDocument& );

            QPersistentModelIndex* m_activePresetIndex;
            QList<APG::PresetPtr> m_presetList;
    }; // class PresetModel

    class PresetModel::ExportDialog : public QFileDialog {
        Q_OBJECT

        public:
            ExportDialog( APG::PresetPtr );
            ~ExportDialog() override;

        Q_SIGNALS:
            void pleaseExport( const QString&, const QList<APG::PresetPtr> ) const;

        private Q_SLOTS:
            void recvAccept() const;

        private:
            QList<APG::PresetPtr> m_presetsToExportList;
    };
} //namespace APG

#endif // APG_PRESET_MODEL
