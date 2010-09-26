/****************************************************************************************
 * Copyright (c) 2010     Ian Monroe <ian@monroe.nu>                                   *
 *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AddServerDialog.h"

#include "ui_NewServerWidget.h"

AddServerDialog::AddServerDialog()
    : KDialog()
    , m_widgets( new Ui::NewServerWidget )
{
    m_widgets->setupUi(this);
    setCaption(i18n("Add new Ampache server"));
    enableButtonOk(false);
    QList<QObject*> inputs;
    inputs << m_widgets->nameLineEdit << m_widgets->serverAddressLineEdit
           << m_widgets->userNameLineEdit << m_widgets-> passwordLineEdit;
    foreach(QObject* line, inputs)
        connect( line, SIGNAL(textEdited(const QString&)), this, SLOT(anyTextEdited()));
}

AddServerDialog::~AddServerDialog()
{
    delete m_widgets;
}

void
AddServerDialog::anyTextEdited()
{
    enableButtonOk(!(name().isEmpty() || url().isEmpty()
                                      || password().isEmpty()
                                      || username().isEmpty() ));
}

QString
AddServerDialog::name()
{
    return m_widgets->nameLineEdit->text();
}

QString
AddServerDialog::url()
{
    return m_widgets->serverAddressLineEdit->text();
}

QString
AddServerDialog::password()
{
    return m_widgets->passwordLineEdit->text();
}

QString
AddServerDialog::username()
{
    return m_widgets->userNameLineEdit->text();
}
