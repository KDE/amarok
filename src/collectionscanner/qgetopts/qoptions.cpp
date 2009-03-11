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
#include "qoptions.h"

static std::string convert(const QString& str)
{
	QByteArray utf8 = str.toUtf8();
	return utf8.data();
}

static std::vector<std::string> convert(const QList<QString>& strList)
{
	std::vector<std::string> result;
	result.reserve(strList.size());
	foreach(QString str, strList)
	{
		result.push_back(convert(str));
	}
	return result;
}

QOptions::QOptions(int argc, const char ** argv)
	: QObject(NULL), m_normalized(false)
{
	if (argc < 0)
	{
		throw QString("Invalid number of arguments: ") + argc;
	}

	for (int i = 0; i < argc; ++i)
	{
		m_arguments += QString(argv[i]);
	}
}

QOptions::QOptions(QStringList arguments)
	: m_arguments(arguments), m_normalized(false)
{
}

QOptions::QOptions(const QOptions& other)
	: QObject(NULL),
      m_arguments(other.m_arguments),
	  m_options(other.m_options),
	  m_normalized(other.m_normalized)
{
}

QOptions::~QOptions()
{
}

void QOptions::normalize()
{
	if (m_normalized)
	{
		return;
	}
	int lastDash = -1;
	for(int i = 1; i < m_arguments.size(); ++i)
	{
		Q_ASSERT(lastDash < i);
		if (m_options.contains(m_arguments[i]))
		{
			if (lastDash != -1)
			{
			}
		}
		else if (lastDash == -1)
		{
			lastDash = i;
		}
	}
	Q_ASSERT(normalized());
	m_normalized = true;
}

bool QOptions::normalized() const
{
	bool dashed = true;
	for(int i = 1; i < m_arguments.size(); ++i)
	{
		if(dashed && !m_arguments[i].startsWith("-"))
		{
			dashed = false;
		}
		else if (!dashed && m_arguments[i].startsWith("-"))
		{
			return false;
		}
	}
	return true;
}

QOptions& QOptions::addOption(const QOption & option)
{
	if (!(option.getLongCode().length() > 2 || option.getShortCode().length() == 2))
	{
		// need to have a valid long code or short code set in the option
		Q_ASSERT(false);
		if (option.getShortCode().isEmpty())
		{
			// ignor the option
			return *this;
		}
		// short option has an invalid length, presumably greater than 0
		if (option.getShortCode().length() > 2)
		{
			QOption corrected(option);
			m_options += corrected.setShortCode(corrected.getShortCode().at(1));
			return *this;
		}
	}

	m_options += option;
	return *this;
}

QOptions& QOptions::operator+=(const QOption& option)
{
	return addOption(option);
}

QOptions QOptions::operator+(const QOption& option) const
{
	QOptions result(*this);
	return result += option;
}

void QOptions::printHelp(QTextStream & output)
{
}

void QOptions::parse()
{
	normalize();
}

QString QOptions::nextOption(bool& shortCode, QStringList& arguments)
{
	normalize();
	return "";
}

QStringList QOptions::nonOptionParameters()
{
	QStringList result;
	return result;
}

std::string QOptions::nextOption(bool& shortCode, std::vector<std::string>& parameters)
{
	QStringList resultParams;
	QString result = nextOption(shortCode, resultParams);
	parameters = convert(resultParams);
	return convert(result);
}

std::vector<std::string> QOptions::nonOptionParameters2()
{
	return convert(nonOptionParameters());
}

#include "qoptions.moc"
