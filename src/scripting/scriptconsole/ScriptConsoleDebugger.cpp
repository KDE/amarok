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

#include "ScriptConsoleDebugger.h"

using namespace ScriptConsoleNS;

ScriptConsoleDebugger::ScriptConsoleDebugger(QObject *parent)
: QObject(parent)
{
    m_errorLogWidget = new QWidget();
    m_debutOutputWidget = new QWidget();
    m_breakpointsWidget = new QWidget();
    m_codeFinderWidget = new QWidget();
    m_codeWidget = new QWidget();
    m_localsWidget = new QWidget();
    m_scriptWidget = new QWidget();
    m_stackWidget = new QWidget();
    m_consoleWidget = new QWidget();
}

ScriptConsoleDebugger::~ScriptConsoleDebugger() {
    delete  m_errorLogWidget;
    delete  m_debutOutputWidget;
    delete  m_breakpointsWidget;
    delete  m_codeFinderWidget;
    delete  m_codeWidget;
    delete  m_localsWidget;
    delete  m_scriptWidget;
    delete  m_stackWidget;
    delete  m_consoleWidget;
}

QAction *ScriptConsoleDebugger::action(ScriptConsoleDebugger::DebuggerAction action) const
{
    switch (action)
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
            break;
    }
    return nullptr;
}

void ScriptConsoleDebugger::ScriptConsoleDebugger::attachTo(QJSEngine *engine)
{
    Q_UNUSED(engine);
}

bool ScriptConsoleDebugger::autoShowStandardWindow() const
{
    return false;
}

QMenu *ScriptConsoleDebugger::createStandardMenu(QWidget *parent)
{
    return new QMenu(parent);
}

QToolBar *ScriptConsoleDebugger::createStandardToolBar(QWidget *parent)
{
    return new QToolBar(parent);
}

void ScriptConsoleDebugger::detach()
{
}

void ScriptConsoleDebugger::setAutoShowStandardWindow(bool autoShow)
{
    Q_UNUSED(autoShow);
}

QMainWindow *ScriptConsoleDebugger::standardWindow() const
{
    return nullptr;
}

ScriptConsoleDebugger::DebuggerState ScriptConsoleDebugger::state() const
{
    return ScriptConsoleDebugger::SuspendedState;
}

QWidget *ScriptConsoleDebugger::widget(ScriptConsoleDebugger::DebuggerWidget widget) const
{
    switch (widget)
    {
        case DebuggerWidget::ConsoleWidget :
            return m_consoleWidget;

        case DebuggerWidget::StackWidget:
            return m_stackWidget;

        case DebuggerWidget::ScriptsWidget:
            return m_scriptWidget;

        case DebuggerWidget::LocalsWidget:
            return m_localsWidget;

        case DebuggerWidget::CodeWidget:
            return m_codeWidget;

        case DebuggerWidget::CodeFinderWidget:
            return m_codeFinderWidget;

        case DebuggerWidget::BreakpointsWidget:
            return m_breakpointsWidget;

        case DebuggerWidget::DebugOutputWidget:
            return m_debutOutputWidget;

        case DebuggerWidget::ErrorLogWidget:
            return m_errorLogWidget;

        default:
            return standardWindow();
    }
}

