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
    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );

    const QLabel *label = nullptr;
    const QLineEdit *field = nullptr;

    for( auto const &obj : widget.children() )
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
    const QString targetName = QStringLiteral("testTargetName");
    SimpleImporterConfigWidget widget( targetName, QVariantMap() );

    const QLineEdit *field = nullptr;
    for( auto const &obj : widget.children() )
        if( qobject_cast<const QLineEdit*>( obj ) )
            field = qobject_cast<const QLineEdit*>( obj );

    QVERIFY( field );
    QCOMPARE( field->text(), targetName );
}

void
TestSimpleImporterConfigWidget::targetNameShouldBeSetToConfigValueIfExists()
{
    const QString targetName = QStringLiteral("nameOverride");

    QVariantMap cfg;
    cfg.insert( "name", targetName );

    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), cfg );

    const QLineEdit *field = nullptr;
    for( auto const &obj : widget.children() )
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
        SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );
        widget.addField( QStringLiteral("configVal"), QStringLiteral("label"), lineEdit.data(), QStringLiteral("text") );

        QVERIFY( !lineEdit.isNull() );
    }

    QVERIFY( lineEdit.isNull() );
}

void
TestSimpleImporterConfigWidget::addFieldShouldAddNewRow()
{
    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );

    QWidget *field = new QLineEdit;
    widget.addField( QStringLiteral("configVal"), QStringLiteral("testLabel"), field, QStringLiteral("text") );

    bool foundField = false;
    bool foundLabel = false;

    for( auto const &obj : widget.children() )
    {
        if( obj == field )
            foundField = true;
        else if( const QLabel *candidate = qobject_cast<const QLabel*>( obj ) )
            if( candidate->text() == QStringLiteral("testLabel") )
                 foundLabel = true;
    }

    QVERIFY( foundField );
    QVERIFY( foundLabel );
}

void
TestSimpleImporterConfigWidget::addFieldShouldAssociateLabelWithField()
{
    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );

    QWidget *field = new QLineEdit;
    widget.addField( QStringLiteral("configVal"), QStringLiteral("testLabel"), field, QStringLiteral("text") );

    const QLabel *label = nullptr;
    for( auto const &obj : widget.children() )
        if( const QLabel *candidate = qobject_cast<const QLabel*>( obj ) )
            if( candidate->text() == QStringLiteral("testLabel") )
                 label = candidate;

    QVERIFY( label );
    QCOMPARE( label->buddy(), field );
}

void
TestSimpleImporterConfigWidget::addFieldShouldNotBreakOnNullField()
{
    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );
    widget.addField( QStringLiteral("configVal"), QStringLiteral("testLabel"), nullptr, QStringLiteral("text") );
}

void
TestSimpleImporterConfigWidget::addedFieldShouldNotModifyFieldValueIfConfigDoesNotExist()
{
    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );

    const QString value = QStringLiteral("myValue");
    QLineEdit *field = new QLineEdit( value );
    widget.addField( QStringLiteral("configVal"), QStringLiteral("testLabel"), field, QStringLiteral("text") );

    QCOMPARE( field->text(), value );
}

void
TestSimpleImporterConfigWidget::addedFieldShouldBeSetToConfigValueIfExists()
{
    const QString configName = QStringLiteral("configVal");
    const QString value = QStringLiteral("myValue");

    QVariantMap cfg;
    cfg.insert( configName, value );
    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), cfg );

    QLineEdit *field = new QLineEdit( QStringLiteral("overrideMe") );
    widget.addField( configName, QStringLiteral("testLabel"), field, QStringLiteral("text") );

    QCOMPARE( field->text(), value );
}

void
TestSimpleImporterConfigWidget::addedFieldShouldNotBreakOnValueSetIfPropertyDoesNotExist()
{
    const QString configName = QStringLiteral("configVal");

    QVariantMap cfg;
    cfg.insert( configName, QStringLiteral("value") );
    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), cfg );

    widget.addField( configName, QStringLiteral("testLabel"), new QLineEdit, QStringLiteral("There's No Such Property") );
}

void
TestSimpleImporterConfigWidget::configShouldContainName()
{
    const QString name = QStringLiteral("testTargetName");
    SimpleImporterConfigWidget widget( name, QVariantMap() );

    QCOMPARE( widget.config().value( QStringLiteral("name") ).toString(), name );
}

void
TestSimpleImporterConfigWidget::configShouldNotBreakOnNullField()
{
    const QString configName = QStringLiteral("configVal");

    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );
    widget.addField( configName, QStringLiteral("testLabel"), nullptr, QStringLiteral("text") );

    QVERIFY( widget.config().value( configName ).toString().isEmpty() );
}

void
TestSimpleImporterConfigWidget::configShouldContainAddedFieldsValues()
{
    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );

    QLineEdit *lineEdit = new QLineEdit;
    lineEdit->setText( QStringLiteral("textValue") );

    QSpinBox *spinBox = new QSpinBox;
    spinBox->setValue( 57 );

    QComboBox *comboBox = new QComboBox;
    comboBox->insertItem( 0, QStringLiteral("item1") );
    comboBox->insertItem( 1, QStringLiteral("item2") );
    comboBox->insertItem( 2, QStringLiteral("item3") );
    comboBox->setCurrentIndex( 1 );

    widget.addField( QStringLiteral("text"), QStringLiteral("text"), lineEdit, QStringLiteral("text") );
    widget.addField( QStringLiteral("int"), QStringLiteral("int"), spinBox, QStringLiteral("value") );
    widget.addField( QStringLiteral("combo"), QStringLiteral("combo"), comboBox, QStringLiteral("currentText") );

    const QVariantMap cfg = widget.config();
    QCOMPARE( cfg.value( QStringLiteral("text") ).toString(), QStringLiteral( "textValue" ) );
    QCOMPARE( cfg.value( QStringLiteral("int") ).toInt(), 57 );
    QCOMPARE( cfg.value( QStringLiteral("combo") ).toString(), QStringLiteral( "item2" ) );
}

void
TestSimpleImporterConfigWidget::configShouldNotBreakOnNonexistentProperty()
{
    const QString configName = QStringLiteral("configName");

    SimpleImporterConfigWidget widget( QStringLiteral("testTargetName"), QVariantMap() );
    widget.addField( configName, QStringLiteral("label"), new QLineEdit, QStringLiteral("No property") );

    QVERIFY( widget.config().value( configName ).toString().isEmpty() );
}
