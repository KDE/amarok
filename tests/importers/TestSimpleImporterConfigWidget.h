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

#ifndef TEST_SIMPLE_IMPORTER_CONFIG_WIDGET
#define TEST_SIMPLE_IMPORTER_CONFIG_WIDGET

#include <QObject>

class TestSimpleImporterConfigWidget : public QObject
{
    Q_OBJECT
public:
    TestSimpleImporterConfigWidget();

private Q_SLOTS:
    void constructorShouldCreateTargetNameRow();
    void targetNameShouldBeSetToDefaultValue();
    void targetNameShouldBeSetToConfigValueIfExists();

    void addFieldShouldTakeFieldOwnership();
    void addFieldShouldAddNewRow();
    void addFieldShouldAssociateLabelWithField();
    void addFieldShouldNotBreakOnNullField();

    void addedFieldShouldNotModifyFieldValueIfConfigDoesNotExist();
    void addedFieldShouldBeSetToConfigValueIfExists();
    void addedFieldShouldNotBreakOnValueSetIfPropertyDoesNotExist();

    void configShouldContainName();
    void configShouldNotBreakOnNullField();
    void configShouldContainAddedFieldsValues();
    void configShouldNotBreakOnNonexistentProperty();
};

#endif // TEST_SIMPLE_IMPORTER_CONFIG_WIDGET
