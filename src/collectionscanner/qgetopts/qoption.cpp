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
#include "qoption.h"

#include <QHash>

QOption::QOption(const QString& category)
	: m_hash(0), m_category(category)
{
}

QOption::QOption(const QOption& other)
	: m_hash(other.m_hash),
	  m_category(other.m_category),
	  m_longCode(other.m_longCode),
	  m_shortCode(other.m_shortCode),
	  m_desc(other.m_desc),
	  m_optionalArgs(other.m_optionalArgs),
	  m_numArgs(other.m_numArgs),
	  m_separator(other.m_separator),
	  m_argumentNames(other.m_argumentNames)
{
}

QOption::~QOption()
{
}

QOption& QOption::setLongCode(const QString& codeName)
{
	Q_ASSERT(!codeName.startsWith("--"));
	Q_ASSERT(!codeName.isEmpty());
	m_longCode.reserve(codeName.length() + 2);
	m_longCode.append("--").append(codeName);
	Q_ASSERT(m_longCode.length() > 2);
	return *this;
}

QOption& QOption::setShortCode(const QChar& code)
{
	Q_ASSERT(code != '-');
	m_shortCode.reserve(2);
	m_shortCode.append("-").append(code);
	Q_ASSERT(m_shortCode.length() == 2);
	return *this;
}

QOption& QOption::setDescription(const QString & description)
{
	m_desc = description;
	return *this;
}

QOption& QOption::setArguments(const int& numArgs, const QChar& separator, const bool& optional, const QStringList& argumentNames)
{
	Q_ASSERT(numArgs >= -1);
	Q_ASSERT(argumentNames.size() == 0 || argumentNames.size() == numArgs);
	m_numArgs = numArgs >= -1 ? numArgs : -1;
	m_separator = separator;
	m_optionalArgs = optional;
	if (m_numArgs == -1)
	{
		m_argumentNames.clear();
		m_argumentNames += "arg1";
		m_argumentNames += "arg2";
		m_argumentNames += "arg3";
		m_argumentNames += "...";
		m_argumentNames += "argN";
	}
	else
	{
		m_argumentNames = argumentNames;
		while (m_argumentNames.size() != m_numArgs)
		{
			m_argumentNames += "arg" + QString::number(m_argumentNames.size());
		}
	}
	return *this;
}

bool QOption::operator==(const QOption& other) const
{
	return m_longCode == other.m_longCode ||
		m_shortCode == other.m_shortCode;
}

bool QOption::operator==(const QString& other) const
{
	return m_longCode == other ||
		QString(m_shortCode) == other;
}

QString QOption::getLongCode() const
{
	return m_longCode;
}

QString QOption::getShortCode() const
{
	return m_shortCode;
}

QString QOption::getDescription() const
{
	return m_desc;
}

QString QOption::argumentsDescription() const
{
	if (m_numArgs == 0)
	{
		return "";
	}

	QString openType = m_optionalArgs ? "[" : "<";
	QString closeType = m_optionalArgs ? "]" : ">";
	QString joiner = m_separator.isSpace() ? "" : m_separator + QString(" ");
	
	return openType + m_argumentNames.join(joiner) + closeType;
}

uint QOption::getHash() const
{
	if (m_hash == 0)
	{
		m_hash = qHash(m_category) + qHash(m_longCode) + qHash(m_shortCode) + qHash(m_numArgs);
	}
	Q_ASSERT(m_hash != 0);
	return m_hash;
}

uint qHash(const QOption& option)
{
	return option.getHash();
}

#include "qoption.moc"
