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

/**
 * SimpleImporterConfigWidget is a helper class for creating non-sophisticated config
 * widgets for importers.
 */
class AMAROK_EXPORT SimpleImporterConfigWidget : public ProviderConfigWidget
{
public:
    /**
     * Constructor. Creates a widget with one label: "Target name," and one text field
     * with its default value specified in @p targetName . @p config contains
     * configuration for this widget.
     * @param targetName the target name
     * @param config configuration for the created widget
     * @param parent the parent widget
     * @param f Qt window flags
     */
    SimpleImporterConfigWidget( const QString &targetName, const QVariantMap &config,
                                QWidget *parent = nullptr, Qt::WindowFlags f = 0 );

    /**
      * Destructor.
      */
    ~SimpleImporterConfigWidget();

    /**
     * addField adds a new row to the widget. @param configName is the name of the config
     * value associated with this field. The row contains a label initialized with
     * @param label and a QWidget @param field initialized with config[configName]
     * (if set). @param property must specify the name of field's property that contains
     * value we want to configure; e.g. for a text field property will be "text", and for
     * a combo box the property may be "currentText" .
     *
     * The ownership of field is transferred to SimpleImporterConfigWidget.
     */
    void addField( const QString &configName, const QString &label,
                   QWidget * const field, const QString &property );

    /**
     * Returns a config generated from this widget's fields.
     */
    QVariantMap config() const override;

private:
    const QVariantMap m_config;
    QMap<QString, QPair<QWidget*, QString> > m_fieldForName;
    QGridLayout *m_layout;
};

} // namespace StatSyncing

#endif // STATSYNCING_SIMPLE_IMPORTER_CONFIG_WIDGET
