/***************************************************************************
                        moodbar.cpp  -  description
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

#include <cmath>
using namespace std;

#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "enginecontroller.h"
#include "sliderwidget.h"
#include "threadweaver.h"
#include "playlist.h"
#include "config.h"
#include "moodbar.h"

#include <qapplication.h>
#include <qbitmap.h>
#include <qbrush.h>
#include <qimage.h>
#include <qpainter.h>
#include <qsize.h>
#include <qtimer.h>
#include <qfile.h>
#include <qmemarray.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kimageeffect.h>
#include <kpixmapeffect.h>
#include <kpopupmenu.h>
#include <kstandarddirs.h>

#ifdef HAVE_EXSCALIBAR
#include <geddei/geddei.h>
using namespace Geddei;

#include <geddei/signaltypes.h>
using namespace SignalTypes;
#endif

amaroK::CreateMood::CreateMood( const QString f )
	: Job( "CreateMood" ), theFilename(f)
{
}

bool amaroK::CreateMood::doJob()
{
	//do some work in thread...
	QString theMoodName = theFilename;
	QString ext = theMoodName.right(3).lower();
	if(ext != "wav" && ext != "mp3" && ext != "ogg")
	{	
		qDebug("CreateMood: Format not recognised.");
		return false;
	}
	theMoodName.truncate(theMoodName.findRev('.'));
	theMoodName += ".mood";
	if(AmarokConfig::moodsWithMusic())
		theMoodName.insert(theMoodName.findRev('/') + 1, '.');
	else
	{	theMoodName.replace('/', ',');
		theMoodName = ::locateLocal("data", "amarok/moods/" + theMoodName);
	}
	if(QFile::exists(theMoodName))
	{	QFile mood(theMoodName);
		if(mood.open(IO_ReadOnly)) return true;
	}
	if(!QFile::exists(theFilename)) return false;
	{	QFile testopen(theFilename);
		if(!testopen.open(IO_ReadOnly)) return false;
	}
#ifdef HAVE_EXSCALIBAR
	qDebug("MakeMood: Creating mood with Exscalibar. Hold onto your hats...");
	ProcessorGroup g;
	ProcessorFactory::create("Player")->init("P", g, Properties("Filename", theFilename));
	SubProcessorFactory::createDom("Mean")->init("M", g);
	SubProcessorFactory::createDom("FFT")->init("F", g, Properties("Size", 1024)("Step", 512));
	SubProcessorFactory::createDom("Bark")->init("B", g);
	SubProcessorFactory::createDom("Fan")->init("S", g, Properties("Multiplicity", 3)("Consolidate", 1));
	MultiProcessor *W = (new MultiProcessor(new SubFactoryCreator("Magnitude"))); W->init("W", g, Properties("Multiplicity", 3));
	MultiProcessor *N = (new MultiProcessor(new FactoryCreator("Normalise"))); N->init("N", g, Properties("Multiplicity", 3));
	ProcessorFactory::create("Dumper")->init("D", g, Properties("Multiplicity", 3)("Pad Before", 0)("Pad After", 0)("Print Section", false)("Print Sample", false)("Print Time", false)("Field Delimiter", " ")("Output", theMoodName));
	g["P"] >>= g["M"];
	g["M"][0] >>= g["F"][0];
	g["F"][0] >>= g["B"][0];
	g["B"][0] >>= g["S"][0];
	g["S"] >>= (*W);
	(*W) >>= (*N);
	(*N) >>= g["D"];
	if(g.go())
	{
		qDebug("MakeMood: Processing...");
		g["D"].waitUntilDone();
		qDebug("MakeMood: Done processing. Cleaning up...");
		g.stop();
	}
	else
		qDebug("MakeMood: Exscalibar reports a problem analysing the song.");
	g.disconnectAll();
	g.deleteAll();
	qDebug("MakeMood: All tidied up.");
	if(!QFile::exists(theMoodName)) return false;
	QFile mood(theMoodName);
	if(!mood.open(IO_ReadOnly)) return false;
	return true;
#else
	return false;
#endif
}

void amaroK::CreateMood::completeJob()
{
	//do completion work in the GUI thread...
	emit completed(theFilename);
}

QValueVector<QColor> amaroK::readMood(const QString path)
{
	qDebug("MakeMood: Reading mood file %s...", path.latin1());
	QString filebase = path;
	QValueVector<QColor> theArray;
	filebase.truncate(filebase.findRev('.'));
	filebase += ".mood";
	QString dotfilebase = filebase, homefilebase = filebase;
	dotfilebase.insert(filebase.findRev('/') + 1, '.');
	homefilebase.replace('/', ',');
	homefilebase = ::locateLocal("data", "amarok/moods/" + homefilebase);
	QFile mood;
	if(QFile::exists(filebase)) mood.setName(filebase);
	if(QFile::exists(dotfilebase)) mood.setName(dotfilebase);
	if(QFile::exists(homefilebase)) mood.setName(homefilebase);
	if(mood.name() != "" && mood.open(IO_ReadOnly))
	{
		qDebug("ReadMood: File opened. Proceeding to read contents...");
		int r, g, b, s = mood.size() / 3;
		QMemArray<int> huedist(360);
		int total = 0, mx = 0;
		for(int i = 0; i < 360; i++) huedist[i] = 0;
		theArray.resize(s);
		for(int i = 0; i < s; i++)
		{	
			r = mood.getch();
			g = mood.getch();
			b = mood.getch();
			theArray[i] = QColor(min(255, max(0, r)), min(255, max(0, g)), min(255, max(0, b)), QColor::Rgb);
			int h, s, v;
			theArray[i].getHsv(&h, &s, &v);
			if(h < 0) h = 0;
			huedist[h]++;
			if(mx < huedist[h]) mx = huedist[h];
		}
		if(AmarokConfig::makeMoodier())
		{
			qDebug("ReadMood: Making moodier!");
//			int threshold = 
//			int rangeStart AmarokConfig::redShades()
			for(int i = 0; i < 360; i++) if(huedist[i] > s / 360 * 5) total++;
			if(total < 360 && total > 0)
			{
				for(int i = 0, n = 0; i < 360; i++)
					huedist[i] = (huedist[i] > s / 360 * 5 ? n++ : n) * 360 / total;
				for(uint i = 0; i < theArray.size(); i++)
				{	int h, s, v;
					theArray[i].getHsv(&h, &s, &v);
					if(h < 0) h = 0;
					theArray[i].setHsv(max(0, min(359, huedist[h])), s, v);
				}
			}
		}
	}
	qDebug("ReadMood: All done.");
	return theArray;
}
