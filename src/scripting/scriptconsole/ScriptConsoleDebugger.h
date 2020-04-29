/*
 * ScriptConsoleDebugger - Wraps a QJSEngine debugger to replace QScriptEngineDebugger
 * Copyright (C) 2020  Pedro de Carvalho Gomes <pedrogomes81@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCRIPTCONSOLEDEBUGGER_H
#define SCRIPTCONSOLEDEBUGGER_H

#include <QActiont>
#include <QJSEngine>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>
#include <QWidget>

/**
 * Wrapper class for debugger that replaces QScriptEngineDebuggger
 */
namespace ScriptConsoleNS
{
    class ScriptConsoleDebugger : public QObject
    {
        Q_OBJECT

    public:

        enum DebuggerWidget {
            ConsoleWidget=0,
            StackWidget=1,
            ScriptsWidget=2,
            LocalsWidget=3,
            CodeWidget=4,
            CodeFinderWidget=5,
            BreakpointsWidget=6,
            DebugOutputWidget=7,
            ErrorLogWidget=8
        };

        enum DebuggerState {
            RunningState=0
            SuspendedState=1
        };

        enum DebuggerAction{
            InterruptAction=0,
            ContinueAction=1,
            StepIntoAction=2,
            StepOverAction=3,
            StepOutAction=4,
            RunToCursorAction=5,
            RunToNewScriptAction=6,
            ToggleBreakpointAction=7,
            ClearDebugOutputAction=8,
            ClearErrorLogAction=9,
            ClearConsoleAction=10,
            FindInScriptAction=11,
            FindNextInScriptAction=12,
            FindPreviousInScriptAction=13,
            GoToLineAction=14
        };

        /**
         * Default constructor
         */
        ScriptConsoleDebugger(QObject *parent = nullptr);

        virtual ~ScriptConsoleDebugger();

        QAction *action(ScriptConsoleDebugger::DebuggerAction action) const;
        void attachTo(QJSEngine *engine);
        bool autoShowStandardWindow() const;
        QMenu *createStandardMenu(QWidget *parent = nullptr);
        QToolBar *createStandardToolBar(QWidget *parent = nullptr);
        void detach();
        void setAutoShowStandardWindow(bool autoShow);
        QMainWindow *standardWindow() const;
        ScriptConsoleDebugger::DebuggerState state() const;
        QWidget *widget(ScriptConsoleDebugger::DebuggerWidget widget) const;

    Q_SIGNALS:
        void evaluationResumed();
        void evaluationSuspended();

    private:

        QWidget m_errorLogWidget;
        QWidget m_debutOutputWidget;
        QWidget m_breakpointsWidget;
        QWidget m_codeFinderWidget;
        QWidget m_codeWidget;
        QWidget m_localsWidget;
        QWidget m_scriptWidget;
        QWidget m_stackWidget;
        QWidget m_consoleWidget;

    };
};

Q_DECLARE_INTERFACE(ScriptConsoleDebugger, "ScriptConsoleDebugger")

#endif // SCRIPTCONSOLEDEBUGGER_H
