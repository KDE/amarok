/****************************************************************************************
 * Copyright (c) 2013 Tatjana Gornak <t.gornak@gmail.com>                               *
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
#include "EncodingSelectionDialog.h"

#include "core/support/Debug.h"

#include <QGroupBox>
#include <QGridLayout>
#include <QDialogButtonBox>
#include <QListWidget>
#include <QLabel>
#include <QScrollArea>
#include <QTextCodec>

EncodingSelectionDialog::EncodingSelectionDialog( const QVector<QString>& encodings,
                                                  const QByteArray& sample,
                                                  QWidget *parent )
                        : QDialog( parent )
{

    QGridLayout *encodingDialog = new QGridLayout;
    QGroupBox *encodingsBox = new QGroupBox();
    encodingsBox->setFlat( 1 );
    encodingsBox->setAlignment( Qt::AlignHCenter );

    QVBoxLayout *layout = new QVBoxLayout( encodingsBox );
    layout->addWidget( new QLabel( tr( "Select the encoding used for the CD's metadata.") ) );
    layout->addStretch( 1 );

    foreach( QString encoding, encodings )
    {
        QString buttonName = encoding;
        if ( !sample.isEmpty() )
        {
            QTextCodec* codec = QTextCodec::codecForName( encoding.toUtf8() );
            if ( codec )
                buttonName = encoding % ": " % codec->toUnicode( sample.data() );
            else
                // we can't use this encoding in a future
                // so it make no sence give it to the user
                continue;
        }
        QRadioButton* button = new QRadioButton( buttonName, this );
        connect( button, SIGNAL(toggled(bool)), this, SLOT(selectionChanged(bool)) );
        layout->addWidget( button );
        m_buttons << button;
    }

    QScrollArea *scroll = new QScrollArea;
    scroll->setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );

    scroll->setWidget( encodingsBox );
    encodingDialog->addWidget( scroll );

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    encodingDialog->addWidget( buttonBox );

    setModal( true );
    setLayout( encodingDialog );
    setWindowTitle( tr( "Encoding Selection Dialog" ) );
    resize( 480, 320 );
}

EncodingSelectionDialog::~EncodingSelectionDialog()
{
}

void EncodingSelectionDialog::selectionChanged( bool checked )
{
    if ( !checked )
        return;
    foreach( QRadioButton* button, m_buttons )
    {
        if ( button == sender() )
        {
            if ( button->text().contains(":") )
                m_selectedEncoding =  button->text().split(":").at(0);
            else
                m_selectedEncoding = button->text();
            break;
        }
    }
}

void EncodingSelectionDialog::accept()
{
    emit encodingSelected( m_selectedEncoding );
    QDialog::accept();
}

#include "EncodingSelectionDialog.moc"

