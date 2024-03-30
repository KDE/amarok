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

#include "kdatecombo.h"


#include <QTimer>
//Added by qt3to4:
#include <QKeyEvent>
#include <QEvent>
#include <QVBoxLayout>


#include <KLocalizedString>
#include <KDatePicker>
#include <KPopupFrame>
#include <QDebug>
#include <KConfigGroup>

KDateCombo::KDateCombo(QWidget *parent) : QComboBox(parent)
{
  setEditable( false );

  QDate date = QDate::currentDate();
  initObject(date);
}

KDateCombo::KDateCombo(const QDate & date, QWidget *parent) : QComboBox(parent)
{
  setEditable( false );

  initObject(date);
}

void KDateCombo::initObject(const QDate & date)
{
  setValidator(nullptr);
  popupFrame = new KPopupFrame(this);
  popupFrame->installEventFilter(this);
  datePicker = new KDatePicker(date, popupFrame);
  datePicker->setMinimumSize(datePicker->sizeHint());
  datePicker->installEventFilter(this);
  popupFrame->setMainWidget(datePicker);
  setDate(date);

  connect(datePicker, &KDatePicker::dateSelected, this, &KDateCombo::dateEnteredEvent);
  connect(datePicker, &KDatePicker::dateEntered, this, &KDateCombo::dateEnteredEvent);
}

KDateCombo::~KDateCombo()
{
  delete datePicker;
  delete popupFrame;
}

QString KDateCombo::date2String(const QDate & date)
{
  return(QLocale().toString(date, QLocale::ShortFormat));
}

QDate & KDateCombo::string2Date(const QString & str, QDate *qd)
{
  return *qd = QLocale().toDate(str, QLocale::ShortFormat);
}

QDate & KDateCombo::getDate(QDate *currentDate)
{
  return string2Date(currentText(), currentDate);
}

bool KDateCombo::setDate(const QDate & newDate)
{
  if (newDate.isValid())
  {
    if (count())
      clear();
    addItem(date2String(newDate));
    return true;
  }
  return false;
}

void KDateCombo::dateEnteredEvent(const QDate &newDate)
{
  QDate tempDate = newDate;
  if (!tempDate.isValid())
     tempDate = datePicker->date();
  popupFrame->hide();
  setDate(tempDate);
}

void KDateCombo::nullDateEnteredEvent()
{
    dateEnteredEvent(QDate());
}

void KDateCombo::mousePressEvent (QMouseEvent * e)
{
  if (e->button() & Qt::LeftButton)
  {
    if  (rect().contains( e->pos()))
    {
      QDate tempDate;
      getDate(& tempDate);
      datePicker->setDate(tempDate);
      popupFrame->popup(mapToGlobal(QPoint(0, height())));
    }
  }
}

bool KDateCombo::eventFilter (QObject*, QEvent* e)
{
  if ( e->type() == QEvent::MouseButtonPress )
  {
      QMouseEvent *me = (QMouseEvent *)e;
      QPoint p = mapFromGlobal( me->globalPos() );
      if (rect().contains( p ) )
      {
          QTimer::singleShot(10, this, &KDateCombo::nullDateEnteredEvent);
        return true;
      }
  }
  else if ( e->type() == QEvent::KeyRelease )
  {
      QKeyEvent *k = (QKeyEvent *)e;

      if (k->key()==Qt::Key_Escape) {
        popupFrame->hide();
        return true;
      }
      else {
        return false;
      }
  }

  return false;
}
