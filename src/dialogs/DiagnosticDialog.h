/****************************************************************************************
 * Copyright (c) 2012 Andrzej J. R. Hunt <andrzej at ahunt.org>                         *
 * Copyright (c) Mark Kretschmann <kretschmann@kde.org>                                 *
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
#ifndef AMAROK_DIAGNOSTICDIALOG_H
#define AMAROK_DIAGNOSTICDIALOG_H

#include "amarok_export.h"

#include <QDialog>

class KAboutData;
class QPlainTextEdit;


class AMAROK_EXPORT DiagnosticDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DiagnosticDialog( const KAboutData about, QWidget *parent = nullptr );

private:
    QPlainTextEdit *m_textBox;

    const QString generateReport( const KAboutData *aboutData );

private Q_SLOTS:
    void slotCopyToClipboard() const;
};


#endif //AMAROK_DIAGNOSTICDIALOG_H
