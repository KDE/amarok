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

#include <config.h>
#include "amarok.h"
#include "amarokconfig.h"
#include "app.h"
#include "debug.h"
#include "enginecontroller.h"
#include "sliderwidget.h"
#include "threadweaver.h"
#include "playlist.h"
#include "moodbar.h"
#include "statusbar.h"

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
	debug() << "MakeMood: Creating mood with Exscalibar. Hold onto your hats..." << endl;
	ProcessorGroup g;
        Processor *proc = ProcessorFactory::create("Player");
        if( !proc )
        {
            g.deleteAll();
            return false;
        }
        proc->init("P", g, Properties("Filename", theFilename));
	if(g["P"].isInitFailed()) { /*amaroK::StatusBar::instance()->longMessageThreadSafe("<strong>Cannot generate Mood data:</strong> Format <em>"+ext+"</em> not supported by your Exscalibar installation.", KDE::StatusBar::Warning);*/ g.deleteAll(); return false; }
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
		debug() << "MakeMood: Processing..." << endl;
		g["D"].waitUntilDone();
		debug() << "MakeMood: Done processing. Stoping..." << flush;
		g.stop();
	}
	debug() << "MakeMood: Disconnecting..." << flush;
	g.disconnectAll();
	debug() << "MakeMood: Deleting..." << flush;
	g.deleteAll();
	debug() << "MakeMood: All tidied up." << endl;
	if(!QFile::exists(theMoodName)) { /*amaroK::StatusBar::instance()->longMessageThreadSafe("<strong>Cannot generate Mood data:</strong> Could not create file (check permissions and Exscalibar installation).", KDE::StatusBar::Warning);*/ return false; }
	QFile mood(theMoodName);
	if(!mood.open(IO_ReadOnly)) { /*amaroK::StatusBar::instance()->longMessageThreadSafe("<strong>Cannot generate Mood data:</strong> Could not verify file (check permissions).", KDE::StatusBar::Warning);*/ return false; }
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
	debug() << "MakeMood: Reading mood file " << path << endl;
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
		int r, g, b, s = mood.size() / 3;
		debug() << "ReadMood: File opened. Proceeding to read contents... s=" << s << endl;
		QMemArray<int> huedist(360);
		int total = 0, mx = 0;
		for(int i = 0; i < 360; i++) huedist[i] = 0;
		theArray.resize(s);
		for(int i = 0; i < s; i++)
		{
			r = mood.getch();
			g = mood.getch();
			b = mood.getch();
			theArray[i] = QColor(CLAMP(0, r, 255), CLAMP(0, g, 255), CLAMP(0, b, 255), QColor::Rgb);
			int h, s, v;
			theArray[i].getHsv(&h, &s, &v);
			if(h < 0) h = 0; else h = h % 360;
			huedist[h]++;
			if(mx < huedist[h]) mx = huedist[h];
		}
		debug() << "ReadMood: File read. Maximum hue bin size = " <<  mx << endl;
		if(AmarokConfig::makeMoodier())
		{
			debug() << "ReadMood: Making moodier!" << endl;
			int threshold, rangeStart = 0, rangeDelta = 359, sat = 100, val = 100;
			switch(AmarokConfig::alterMood())
			{
				// Angry
				case 1: threshold = s / 360 * 9; rangeStart = 45; rangeDelta = -45; sat = 200; val = 100; break;
				// Frozen
				case 2: threshold = s / 360 * 1; rangeStart = 140; rangeDelta = 160; sat = 50; val = 100; break;
				// Happy
				default: threshold = s / 360 * 2; rangeStart = 0; rangeDelta = 359; sat = 150; val = 250;
			}
			debug() << "ReadMood: Appling filter t=" << threshold << ", rS=" << rangeStart << ", rD=" << rangeDelta << ", s=" << sat << "%, v=" << val << "%" << endl;
			for(int i = 0; i < 360; i++) if(huedist[i] > threshold) total++;
			debug() << "ReadMood: Total=" << total << endl;
			if(total < 360 && total > 0)
			{
				for(int i = 0, n = 0; i < 360; i++)
					huedist[i] = ((huedist[i] > threshold ? n++ : n) * rangeDelta / total + rangeStart) % 360;
				for(uint i = 0; i < theArray.size(); i++)
				{	int h, s, v;
					theArray[i].getHsv(&h, &s, &v);
					if(h < 0) h = 0; else h = h % 360;
					if(h > 359) debug() << "ReadMood: Bad hue in array[" << i << "]: " << h << endl;
					theArray[i].setHsv(CLAMP(0, huedist[h], 359), CLAMP(0, s * sat / 100, 255), CLAMP(0, v * val / 100, 255));
				}
			}
		}
	}
	debug() << "ReadMood: All done." << endl;
	return theArray;
}


#include "moodbar.moc"
