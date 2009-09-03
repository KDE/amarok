/****************************************************************************************
 * Copyright (c) 2008 Daniel Caleb Jones <danielcjones@gmail.com>                       *
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

#ifndef KDATECOMBO_H
#define KDATECOMBO_H

#include <QtGui/QWidget>
#include <QtGui/QComboBox>
#include <QtCore/QDate>

/**
  *@author Beppe Grimaldi
  */

class KDatePicker;
class KPopupFrame;

class KDateCombo : public QComboBox  {
   Q_OBJECT

public:
	KDateCombo(QWidget *parent=0);
	explicit KDateCombo(const QDate & date, QWidget *parent=0);
	~KDateCombo();

	QDate & getDate(QDate *currentDate);
	bool setDate(const QDate & newDate);

private:
   KPopupFrame * popupFrame;
   KDatePicker * datePicker;

   void initObject(const QDate & date);

   QString date2String(const QDate &);
   QDate & string2Date(const QString &, QDate * );

protected:
  bool eventFilter (QObject*, QEvent*);
  virtual void mousePressEvent (QMouseEvent * e);

protected Q_SLOTS:
   void dateEnteredEvent(const QDate &d=QDate());
};

#endif
