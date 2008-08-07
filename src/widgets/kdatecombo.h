/***********************************************************************
 *
 *  kdatecombo.h
 *
 ***********************************************************************/

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
