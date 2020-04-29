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

#include "scriptconsoledebugger.h"

ScriptConsoleDebugger::ScriptConsoleDebugger(QObject *parent) : QObject(parent)
{
    m_errorLogWidget = new QWidget( this );
    m_debutOutputWidget = new QWidget( this );
    m_breakpointsWidget = new QWidget( this );
    m_codeFinderWidget = new QWidget( this );
    m_codeWidget = new QWidget( this );
    m_localsWidget = new QWidget( this );
    m_scriptWidget = new QWidget( this );
    m_stackWidget = new QWidget( this );
    m_consoleWidget = new QWidget( this );
}

ScriptConsoleDebugger::~ScriptConsoleDebugger() {
}

QAction *ScriptConsoleDebugger::action(ScriptConsoleDebugger::DebuggerAction action) const
{
    swtich (action)
    {
        case DebuggerAction::InterruptAction:
        case DebuggerAction::ContinueAction:
        case DebuggerAction::StepIntoAction:
        case DebuggerAction::StepOverAction:
        case DebuggerAction::StepOutAction:
        case DebuggerAction::RunToCursorAction:
        case DebuggerAction::RunToNewScriptAction:
        case DebuggerAction::ToggleBreakpointAction:
        case DebuggerAction::ClearDebugOutputAction:
        case DebuggerAction::ClearErrorLogAction:
        case DebuggerAction::ClearConsoleAction:
        case DebuggerAction::FindInScriptAction:
        case DebuggerAction::FindNextInScriptAction:
        case DebuggerAction::FindPreviousInScriptAction:
        case DebuggerAction::GoToLineAction:
    }
    return nullptr;
}

void ScriptConsoleDebugger::ScriptConsoleDebugger::attachTo(QJSEngine *engine)
{
}

bool ScriptConsoleDebugger::autoShowStandardWindow() const
{
    return false;
}

QMenu *ScriptConsoleDebugger::createStandardMenu(QWidget *parent = nullptr)
{
    return new QToolBar(parent);
}

QToolBar *ScriptConsoleDebugger::createStandardToolBar(QWidget *parent = nullptr)
{
    return new QToolBar(parent);
}

void ScriptConsoleDebugger::detach()
{
}

void ScriptConsoleDebugger::setAutoShowStandardWindow(bool autoShow)
{
}

QMainWindow *ScriptConsoleDebugger::standardWindow() const
{
    return nullptr;
}

ScriptConsoleDebugger::DebuggerState ScriptConsoleDebugger::state() const
{
    return 0;
}

QWidget *ScriptConsoleDebugger::widget(ScriptConsoleDebugger::DebuggerWidget widget) const
{
    switch (widget)
    {
        DebuggerWidget::ConsoleWidget:
            return m_consoleWidget;

        DebuggerWidget::StackWidget:
            return m_stackWidget;

        DebuggerWidget::ScriptsWidget:
            return m_scriptWidget;

        DebuggerWidget::LocalsWidget:
            return m_localsWidget;

        DebuggerWidget::CodeWidget:
            return m_codeWidget;

        DebuggerWidget::CodeFinderWidget:
            return m_codeFinderWidget;

        DebuggerWidget::BreakpointsWidget:
            return m_breakpointsWidget;

        DebuggerWidget::DebugOutputWidget:
            return m_debutOutputWidget;

        DebuggerWidget::ErrorLogWidget:
            return m_errorLogWidget;

        default:
            return standardWindow();
    }
}

