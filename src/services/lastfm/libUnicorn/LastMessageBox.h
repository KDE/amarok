/***************************************************************************
*   Copyright (C) 2007 by                                                 *
*      Philipp Maihart, Last.fm Ltd <phil@last.fm>                        *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
***************************************************************************/

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include "UnicornDllExportMacro.h"

#include <QMessageBox>


class UNICORN_DLLEXPORT LastMessageBox : public QMessageBox
{
    Q_OBJECT

public:
    LastMessageBox( QWidget* parent = 0 );
    LastMessageBox( Icon icon,
                    const QString& title,
                    const QString& text,
                    StandardButtons buttons = NoButton,
                    QWidget* parent = 0,
                    Qt::WindowFlags f = Qt::Dialog | Qt::MSWindowsFixedSizeDialogHint,
                    QStringList buttonTexts = QStringList() );

    virtual void setText ( const QString& text );
    virtual void setWindowTitle ( const QString& title );

    //FIXME wtf? Hardcoded?
    virtual QSize sizeHint() const { return QSize( 480, 153 ); }

    static QMessageBox::StandardButton
    critical( const QString& title,
              const QString& text,
              StandardButtons buttons = Ok,
              StandardButton defaultButton = NoButton,
              QStringList buttonTexts = QStringList(),
              QWidget* parent = 0 );

    static QMessageBox::StandardButton
    information( const QString& title,
                 const QString & text,
                 StandardButtons buttons = Ok,
                 StandardButton defaultButton = NoButton,
                 QStringList buttonTexts = QStringList(),
                 QWidget* parent = 0 );

    static QMessageBox::StandardButton
    question( const QString& title,
              const QString& text,
              StandardButtons buttons = Ok,
              StandardButton defaultButton = NoButton,
              QStringList buttonTexts = QStringList(),
              QWidget* parent = 0 );

    static QMessageBox::StandardButton
    warning( const QString& title,
             const QString& text,
             StandardButtons buttons = Ok,
             StandardButton defaultButton = NoButton,
             QStringList buttonTexts = QStringList(),
             QWidget* parent = 0 );
};


#ifdef Q_WS_MAC
QMessageBox::StandardButton
showMacMessageBox(QWidget *parent, QMessageBox::Icon icon,
                  const QString& title, const QString& text,
                  QMessageBox::StandardButtons buttons,
                  QMessageBox::StandardButton defaultButton,
                  QStringList buttonTexts = QStringList()
                  );
#endif

#endif // MESSAGEBOX_H
