/***************************************************************************
                       moodbar.h  -  description
                          -------------------
 begin                : 6th Nov 2005
 copyright            : (C) 2005 by Gav Wood
 email                : gav@kde.org
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef MOODBAR_H
#define MOODBAR_H

#include <qcolor.h>
#include <qvaluevector.h>
#include <qslider.h>
#include <qvaluevector.h>
#include <qpixmap.h>
#include <kpixmap.h>
#include <kurl.h>

#include "metabundle.h"
#include "threadweaver.h"
#include "engineobserver.h"

#ifdef HAVE_EXSCALIBAR
#define WANT_MOODBAR AmarokConfig::showMoodbar()
#else
#define WANT_MOODBAR false
#endif

#define CLAMP(n, v, x) ((v) < (n) ? (n) : (v) > (x) ? (x) : (v))

class QPalette;
class QTimer;

namespace amaroK
{

QValueVector<QColor> readMood(const QString path);

class CreateMood : public ThreadWeaver::Job
{
Q_OBJECT
	QString theFilename;
signals:
	void completed(const QString);
public:
	CreateMood( const QString f );

	virtual bool doJob();
	virtual void completeJob();
};

}

#endif
