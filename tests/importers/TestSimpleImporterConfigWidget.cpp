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

#include "TestSimpleImporterConfigWidget.h"

#include "importers/SimpleImporterConfigWidget.h"

#include <KLocalizedString>

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTest>


QTEST_MAIN( TestSimpleImporterConfigWidget )

using namespace StatSyncing;

TestSimpleImporterConfigWidget::TestSimpleImporterConfigWidget()
{
    KLocalizedString::setApplicationDomain("amarok-test");
}

void
TestSimpleImporterConfigWidget::constructorShouldCreateTargetNameRow()
{
    SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );

    const QLabel *label = nullptr;
    const QLineEdit *field = nullptr;

    foreach( const QObject *obj, widget.children() )
    {
        if( qobject_cast<const QLabel*>( obj ) )
            label = qobject_cast<const QLabel*>( obj );
        else if( qobject_cast<const QLineEdit*>( obj ) )
            field = qobject_cast<const QLineEdit*>( obj );
    }

    QVERIFY( label );
    QVERIFY( field );

    QVERIFY( !label->text().isEmpty() );
    QCOMPARE( label->buddy(), field );
}

void
TestSimpleImporterConfigWidget::targetNameShouldBeSetToDefaultValue()
{
    const QString targetName = "testTargetName";
    SimpleImporterConfigWidget widget( targetName, QVariantMap() );

    const QLineEdit *field = nullptr;
    foreach( const QObject *obj, widget.children() )
        if( qobject_cast<const QLineEdit*>( obj ) )
            field = qobject_cast<const QLineEdit*>( obj );

    QVERIFY( field );
    QCOMPARE( field->text(), targetName );
}

void
TestSimpleImporterConfigWidget::targetNameShouldBeSetToConfigValueIfExists()
{
    const QString targetName = "nameOverride";

    QVariantMap cfg;
    cfg.insert( "name", targetName );

    SimpleImporterConfigWidget widget( "testTargetName", cfg );

    const QLineEdit *field = nullptr;
    foreach( const QObject *obj, widget.children() )
        if( qobject_cast<const QLineEdit*>( obj ) )
            field = qobject_cast<const QLineEdit*>( obj );

    QVERIFY( field );
    QCOMPARE( field->text(), targetName );
}

void
TestSimpleImporterConfigWidget::addFieldShouldTakeFieldOwnership()
{
    QPointer<QWidget> lineEdit( new QLineEdit() );

    {
        SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );
        widget.addField( "configVal", "label", lineEdit.data(), "text" );

        QVERIFY( !lineEdit.isNull() );
    }

    QVERIFY( lineEdit.isNull() );
}

void
TestSimpleImporterConfigWidget::addFieldShouldAddNewRow()
{
    SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );

    QWidget *field = new QLineEdit;
    widget.addField( "configVal", "testLabel", field, "text" );

    bool foundField = false;
    bool foundLabel = false;

    foreach( const QObject *obj, widget.children() )
    {
        if( obj == field )
            foundField = true;
        else if( const QLabel *candidate = qobject_cast<const QLabel*>( obj ) )
            if( candidate->text() == "testLabel" )
                 foundLabel = true;
    }

    QVERIFY( foundField );
    QVERIFY( foundLabel );
}

void
TestSimpleImporterConfigWidget::addFieldShouldAssociateLabelWithField()
{
    SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );

    QWidget *field = new QLineEdit;
    widget.addField( "configVal", "testLabel", field, "text" );

    const QLabel *label = nullptr;
    foreach( const QObject *obj, widget.children() )
        if( const QLabel *candidate = qobject_cast<const QLabel*>( obj ) )
            if( candidate->text() == "testLabel" )
                 label = candidate;

    QVERIFY( label );
    QCOMPARE( label->buddy(), field );
}

void
TestSimpleImporterConfigWidget::addFieldShouldNotBreakOnNullField()
{
    SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );
    widget.addField( "configVal", "testLabel", nullptr, "text" );
}

void
TestSimpleImporterConfigWidget::addedFieldShouldNotModifyFieldValueIfConfigDoesNotExist()
{
    SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );

    const QString value = "myValue";
    QLineEdit *field = new QLineEdit( value );
    widget.addField( "configVal", "testLabel", field, "text" );

    QCOMPARE( field->text(), value );
}

void
TestSimpleImporterConfigWidget::addedFieldShouldBeSetToConfigValueIfExists()
{
    const QString configName = "configVal";
    const QString value = "myValue";

    QVariantMap cfg;
    cfg.insert( configName, value );
    SimpleImporterConfigWidget widget( "testTargetName", cfg );

    QLineEdit *field = new QLineEdit( "overrideMe" );
    widget.addField( configName, "testLabel", field, "text" );

    QCOMPARE( field->text(), value );
}

void
TestSimpleImporterConfigWidget::addedFieldShouldNotBreakOnValueSetIfPropertyDoesNotExist()
{
    const QString configName = "configVal";

    QVariantMap cfg;
    cfg.insert( configName, "value" );
    SimpleImporterConfigWidget widget( "testTargetName", cfg );

    widget.addField( configName, "testLabel", new QLineEdit, "There's No Such Property" );
}

void
TestSimpleImporterConfigWidget::configShouldContainName()
{
    const QString name = "testTargetName";
    SimpleImporterConfigWidget widget( name, QVariantMap() );

    QCOMPARE( widget.config().value( "name" ).toString(), name );
}

void
TestSimpleImporterConfigWidget::configShouldNotBreakOnNullField()
{
    const QString configName = "configVal";

    SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );
    widget.addField( configName, "testLabel", nullptr, "text" );

    QVERIFY( widget.config().value( configName ).toString().isEmpty() );
}

void
TestSimpleImporterConfigWidget::configShouldContainAddedFieldsValues()
{
    SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );

    QLineEdit *lineEdit = new QLineEdit;
    lineEdit->setText( "textValue" );

    QSpinBox *spinBox = new QSpinBox;
    spinBox->setValue( 57 );

    QComboBox *comboBox = new QComboBox;
    comboBox->insertItem( 0, "item1" );
    comboBox->insertItem( 1, "item2" );
    comboBox->insertItem( 2, "item3" );
    comboBox->setCurrentIndex( 1 );

    widget.addField( "text", "text", lineEdit, "text" );
    widget.addField( "int", "int", spinBox, "value" );
    widget.addField( "combo", "combo", comboBox, "currentText" );

    const QVariantMap cfg = widget.config();
    QCOMPARE( cfg.value( "text" ).toString(), QString( "textValue" ) );
    QCOMPARE( cfg.value( "int" ).toInt(), 57 );
    QCOMPARE( cfg.value( "combo" ).toString(), QString( "item2" ) );
}

void
TestSimpleImporterConfigWidget::configShouldNotBreakOnNonexistentProperty()
{
    const QString configName = "configName";

    SimpleImporterConfigWidget widget( "testTargetName", QVariantMap() );
    widget.addField( configName, "label", new QLineEdit, "No property" );

    QVERIFY( widget.config().value( configName ).toString().isEmpty() );
}
