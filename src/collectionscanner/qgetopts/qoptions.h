
#ifndef QOPTIONS_H
#define QOPTIONS_H
/***************************************************************************
 *   Copyright (C) 2008 by Vitali Lovich   *
 *   vlovich@gmail.com   *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include <QObject>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QSet>

#include <string>
#include <vector>

#include "qoption.h"

/**
 * @author Vitali Lovich <vlovich@gmail.com>
 */
class QOptions : public QObject
{
	Q_OBJECT
public:
	QOptions(int argc, const char ** argv);
	QOptions(QStringList arguments);
	QOptions(const QOptions& other);
	~QOptions();

	QString nextOption(bool& shortCode, QStringList& parameters);
	QStringList nonOptionParameters();

	std::string nextOption(bool& shortCode, std::vector<std::string>& parameters);
	std::vector<std::string> nonOptionParameters2();

	QOptions& operator+=(const QOption& option);
	QOptions operator+(const QOption& option) const;

public slots:
	QOptions& addOption(const QOption & option);
	void printHelp(QTextStream& output);
	void parse();	

signals:
	void foundOption(QOption & option, QStringList args) const;
	void nonOptionValue(QString value) const;
	void parsingError(QString reason) const;

private:
	void normalize();
	bool normalized() const;

	QStringList m_arguments;
	QSet<QOption> m_options;
	bool m_normalized;
};

#endif /* QOPTIONS_H */
