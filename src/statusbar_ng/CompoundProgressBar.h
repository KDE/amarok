/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#ifndef COMPOUNDPROGRESSBAR_H
#define COMPOUNDPROGRESSBAR_H

#include "statusbar_ng/ProgressBar.h"
#include "statusbar_ng/PopupWidget.h"

#include <QList>
#include <QMap>

/**
A progress bar that wraps a number of simple progress ars and displays their overall progress. Also features an expanded mode that allows the user to view and canceld individual operations

	@author
*/
class AMAROK_EXPORT CompoundProgressBar : public ProgressBarNG
{
    Q_OBJECT
public:
    CompoundProgressBar( QWidget * parent );

    ~CompoundProgressBar();

    void addProgressBar( ProgressBarNG * progressBar, QObject *owner );
    void endProgressOperation( const QObject * owner );

    void incrementProgress( const QObject *owner );
    void incrementProgressTotalSteps( const QObject *owner, int inc = 1 );
    void setProgressStatus( const QObject *owner, const QString &text );
    void setProgress( const QObject *owner, int steps );

signals:
    void allDone();

protected slots:
    void cancelAll();
    void toggleDetails();

    void childPercentageChanged( );
    void childBarCancelled( ProgressBarNG * progressBar );
    void childBarComplete( ProgressBarNG * progressBar );

private:
    void showDetails();
    void hideDetails();

    void handleDetailsButton();

    int calcCompoundPercentage();

    QMap<const QObject *, ProgressBarNG *> m_progressMap;

    QToolButton * m_showDetailsButton;

    PopupWidget * m_progressDetailsWidget;
};

#endif
