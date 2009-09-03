/****************************************************************************************
 * Copyright (c) 2009 Thomas Lübking <thomas.luebking@web.de                            *
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
 
#ifndef PLAYLISTLAYOUTDIALOG_H
#define PLAYLISTLAYOUTDIALOG_H

#include <QDialog>
#include <QPointer>

class HintingLineEdit;
class QLineEdit;
class QLabel;
class QRadioButton;
class QSlider;
class QToolButton;
class TokenWithLayout;

class LayoutEditDialog : public QDialog
{
    Q_OBJECT
public:
    LayoutEditDialog( QWidget *parent = 0 );
    void setToken( TokenWithLayout *t );
public slots:
    void close();
private slots:
    void apply();
    void setPeerWidth( bool peer );
private:
    QPointer<TokenWithLayout> m_token;
    HintingLineEdit *m_prefix, *m_suffix;
    QLabel *m_element;
    QSlider *m_width;
    int m_previousWidth;
    QToolButton *m_bold, *m_italic, *m_alignLeft, *m_alignCenter, *m_alignRight;
    QRadioButton *m_fixedWidth, *m_fitContent, *m_peerWidth;
};

#endif
