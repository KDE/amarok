/****************************************************************************************
 * Copyright (c) 2010 Andrew Coder <andrew.coder@gmail.com>                             *
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

#ifndef QUERYMAKER_FUNCTION_TYPES_H
#define QUERYMAKER_FUNCTION_TYPES_H

#include "core/collections/QueryMaker.h"

using namespace Collections;

/**
 * A CurriedQMFunction is a call to any of the member functions
 * of QueryMaker that return a QueryMaker*, which it's arguments
 * specified, but missing, (normally implicit), this pointer parameter.
 *
 * Subclasses provide a typedef named FunPtr as a convenience, which
 * provides a type equal to that of a pointer to a QueryMaker member
 * function which returns a QueryMaker* and accepts the number and
 * type of parameters that the CurriedQMFunction does. Having the
 * typedefs in the classes prevents issues with C++'s lack of support
 * for template typedefs.
 */
class CurriedQMFunction
{
    public:
        virtual ~CurriedQMFunction() {};
        /**
         * Invokes the stored function, with the stored
         * parameters, providing @param qm as the this pointer.
         * @return qm
         */
        virtual QueryMaker* operator()( QueryMaker *qm = 0 ) = 0;
};

/**
 * The CurriedQMFunction for functions with no arguments.
 */
class CurriedZeroArityQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ();

        CurriedZeroArityQMFunction( FunPtr function )
            : m_function( function )
        {};
        virtual ~CurriedZeroArityQMFunction() {};

        QueryMaker* operator()( QueryMaker *qm = 0 )
        {
            if( qm )
                return ( qm->*m_function )();
            return qm;
        };

    private:
        FunPtr m_function;
};

/**
 * The CurriedQMFunction for functions with one arguments.
 */
template< class Type >
class CurriedUnaryQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( Type );

        CurriedUnaryQMFunction( FunPtr function, Type parameter )
            : m_function( function )
            , m_parameter( parameter )
        {};
        virtual ~CurriedUnaryQMFunction() {};

        QueryMaker* operator()( QueryMaker *qm )
        {
            if( qm )
                return ( qm->*m_function )( m_parameter );
            return qm;
        };

    private:
        FunPtr m_function;
        Type m_parameter;
};

/**
 * The CurriedQMFunction for functions with two arguments.
 */
template< class FirstType, class SecondType >
class CurriedBinaryQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( FirstType, SecondType );

        CurriedBinaryQMFunction( FunPtr function, FirstType parameterOne, SecondType parameterTwo )
            : m_function( function )
            , m_parameterOne( parameterOne )
            , m_parameterTwo( parameterTwo )
        {};
        virtual ~CurriedBinaryQMFunction() {};

        QueryMaker* operator()( QueryMaker *qm )
        {
            if( qm )
                return ( qm->*m_function )( m_parameterOne, m_parameterTwo );
            return qm;
        };

    private:
        FunPtr m_function;
        FirstType m_parameterOne;
        SecondType m_parameterTwo;
};

/**
 * The CurriedQMFunction for functions with three arguments.
 */
template< class FirstType, class SecondType, class ThirdType >
class CurriedTrinaryQMFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( FirstType, SecondType, ThirdType );

        CurriedTrinaryQMFunction( FunPtr function, FirstType parameterOne, SecondType parameterTwo, ThirdType parameterThree )
            : m_function( function )
            , m_parameterOne( parameterOne )
            , m_parameterTwo( parameterTwo )
            , m_parameterThree( parameterThree )
        {};
        virtual ~CurriedTrinaryQMFunction() {};

        QueryMaker* operator()( QueryMaker *qm )
        {
            if( qm )
                return ( qm->*m_function )( m_parameterOne, m_parameterTwo, m_parameterThree );
            return qm;
        };

    private:
        FunPtr m_function;
        FirstType m_parameterOne;
        SecondType m_parameterTwo;
        ThirdType m_parameterThree;
};

/**
 * The CurriedQMFunction for functions with four arguments.
 * Passing the QString as a reference to the constructor caused
 * some problems, so this one's specialized for the only two
 * members of QueryMaker that have four parameters, addFilter
 * and excludeFilter.
 */
class CurriedQMStringFilterFunction : public CurriedQMFunction
{
    public:
        typedef QueryMaker* ( QueryMaker::*FunPtr ) ( qint64, const QString&, bool, bool );

        CurriedQMStringFilterFunction( FunPtr function, qint64 value, QString filter, bool matchBegin, bool matchEnd )
            : m_function( function )
            , m_value( value )
            , m_filter( filter )
            , m_matchBegin( matchBegin )
            , m_matchEnd( matchEnd )
        {};
        virtual ~CurriedQMStringFilterFunction() {};

        QueryMaker* operator()( QueryMaker *qm )
        {
            if( qm )
                return ( qm->*m_function )( m_value, m_filter, m_matchBegin, m_matchEnd );
            return qm;
        };

    private:
        FunPtr m_function;
        qint64 m_value;
        QString m_filter;
        bool m_matchBegin;
        bool m_matchEnd;
};

#endif
