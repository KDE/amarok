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
#ifndef ENCODINGSELECTIONDIALOG_H
#define ENCODINGSELECTIONDIALOG_H

#include <QDialog>
#include <QRadioButton>


/**
 * Asks user to choose an encoding from list
 * of encoding guessed for collection.
 */
class EncodingSelectionDialog
    : public QDialog
{
    Q_OBJECT
public:
    EncodingSelectionDialog( const QVector<QString>& encodings, const QByteArray& sample, QWidget *parent = 0 );
    ~EncodingSelectionDialog();

public slots:
    virtual void accept();
signals:
    void encodingSelected( QString& );
private slots:
    void selectionChanged( bool checked );
private:
    QString m_selectedEncoding;
    QList<QRadioButton*> m_buttons;
};

#endif
