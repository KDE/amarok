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

#include "AmpacheAccountLogin.h"
#include "ui_NewServerWidget.h"

AddServerDialog::AddServerDialog()
    : KDialog()
    , m_widgets( new Ui::NewServerWidget )
{
    QWidget* widget = new QWidget();
    m_widgets->setupUi(widget);
    setMainWidget(widget);
    
    m_widgets->verifyButton->setEnabled(false);
    setCaption(i18n("Add new Ampache server"));
    enableButtonOk(false);
    
    connect( m_widgets->verifyButton, SIGNAL(released()), this, SLOT(verifyData()));
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
   bool minimumData = (!(name().isEmpty() || url().isEmpty()
                                      || password().isEmpty()
                                      || username().isEmpty() ));
   enableButtonOk(minimumData);
   m_widgets->verifyButton->setEnabled(minimumData);
}

void AddServerDialog::verifyData()
{
    m_widgets->verifyButton->setEnabled(false);
    delete m_login; //should always be null at this point.
    m_login = new AmpacheAccountLogin( url(), username(), password(), this );
    connect(m_login, SIGNAL(finished()), this, SLOT(loginResult()));
}

void AddServerDialog::loginResult()
{
    QLabel* label = m_widgets->verifyLabel;
    QPalette pal = label->palette();
    if( m_login->authenticated() )
    {
        label->setText( i18n("Successfully connected") );
        pal.setColor( QPalette::WindowText, Qt::darkGreen );
    }
    else
    {
        label->setText( i18n("Connection failure") );
        pal.setColor( QPalette::WindowText, Qt::red );
    }
    label->setPalette(pal);
    delete m_login;
    m_widgets->verifyButton->setEnabled(true);
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
