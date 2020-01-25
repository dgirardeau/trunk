//##########################################################################
//#                                                                        #
//#                CLOUDCOMPARE PLUGIN: ChaiScriptingPlugin                #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 of the License.               #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#                     COPYRIGHT: Chris S Brown                           #
//#                                                                        #
//##########################################################################


#include <QtGui>

#include "ChaiScriptingPlugin.h"

#include <chaiscript/chaiscript.hpp>
#include <chaiscript/chaiscript_stdlib.hpp>
#include <chaiscript/dispatchkit/bootstrap_stl.hpp>
#include <chaiscript/dispatchkit/function_call.hpp>


#include "chaiScriptCodeEditorMainWindow.h"


chaiscript::ChaiScript* chai;
	/// \brief Represents the current state of the ChaiScript system. State and be saved and restored
	/// \warning State object does not contain the user defined type conversions of the engine. They
	///          are left out due to performance considerations involved in tracking the state
	/// \sa ChaiScript::get_state
	/// \sa ChaiScript::set_state
chaiscript::ChaiScript::State chaiInitialState;
chaiscript::ChaiScript::State chaiSaveState;

chaiScriptCodeEditorMainWindow* cseMW;

using namespace chaiscript;
ChaiScriptingPlugin::ChaiScriptingPlugin( QObject *parent )
	: QObject( parent )
	, ccStdPluginInterface( ":/CC/plugin/ChaiScriptingPlugin/info.json" )
	, m_action( nullptr )
{
	cseMW = chaiScriptCodeEditorMainWindow::TheInstance();
	setupChaiScriptEngine();
	connect(cseMW, &chaiScriptCodeEditorMainWindow::executionCalled, this, &ChaiScriptingPlugin::executionCalled);
	connect(cseMW, &chaiScriptCodeEditorMainWindow::reset_Chai_to_initial_state, this, &ChaiScriptingPlugin::resetToDefaultChaiState);
	connect(cseMW, &chaiScriptCodeEditorMainWindow::save_Chai_state, this, &ChaiScriptingPlugin::saveChaiState);
	connect(cseMW, &chaiScriptCodeEditorMainWindow::reset_chai_to_last_save, this, &ChaiScriptingPlugin::resetToSavedChaiState);
}

bool throws_exception(const std::function<void()>& f)
{
	try {
		f();
	}
	catch (...) {
		return true;
	}

	return false;
}

const QString stdStringToQString(const std::string &str)
{
	return QString::fromUtf8(str.c_str());
}

chaiscript::exception::eval_error get_eval_error(const std::function<void()>& f)
{
	try {
		f();
	}
	catch (const chaiscript::exception::eval_error & e) {
		return e;
	}

	throw std::runtime_error("no exception throw");
}

void ChaiScriptingPlugin::setupChaiScriptEngine()
{
	try 
	{
		if (!chai)
		{
			chai = new ChaiScript(chaiscript::Std_Lib::library());
			
			chai->add(chaiscript::type_conversion<std::string, QString>([](const std::string& str) {return QString::fromStdString(str); }));
			chai->add(chaiscript::vector_conversion<std::vector<int>>());
			chai->add(chaiscript::map_conversion<std::map<std::string, int>>());

			chai->add_global(var(this), "chaiScriptingPlugin");
			chai->add_global(var(m_app), "m_app");
			chai->add(fun(&ccMainAppInterface::addToDB), "addToDB");
			chai->add(fun([=]() { return m_app->haveSelection(); }), "haveSelection");
			chai->add(fun([=]() { return m_app->addToDB(); }), "haveSelection");
			
			chai->add(fun(&ChaiScriptingPlugin::dispToConsole), "dispToConsole");
			chai->add(chaiscript::user_type<ccMainAppInterface>(), "ccMainAppInterface");
			chai->add(chaiscript::user_type<QString>(), "QString");
			chai->add(chaiscript::fun(&throws_exception), "throws_exception");
			chai->add(chaiscript::fun(&get_eval_error), "get_eval_error");
			chai->eval(R"(global print = fun(x){chaiScriptingPlugin.dispToConsole(x.to_string(),1);})");
			chai->eval(R"(def method_missing(int i, string name, Vector v) {
					print("method_missing(${i}, ${name}), ${v.size()} params");
					})");
			chaiInitialState = chai->get_state();
		}
		

	}
	catch (const chaiscript::exception::eval_error & ee) {
		m_app->dispToConsole(ee.what(), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		if (ee.call_stack.size() > 0) {
			m_app->dispToConsole(QString("during evaluation at (%1, %2)").arg(ee.call_stack[0]->start().line).arg(ee.call_stack[0]->start().column), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
			//std::cout << "during evaluation at (" << ee.call_stack[0]->start().line << ", " << ee.call_stack[0]->start().column << ")";
		}
		std::cout << std::endl;
	}
	catch (const std::exception & e) {
		m_app->dispToConsole(e.what(), ccMainAppInterface::ERR_CONSOLE_MESSAGE);

	}

}


void ChaiScriptingPlugin::dispToConsole(const std::string &str,const int lvl)
{
	m_app->dispToConsole(QString::fromStdString(str), (ccMainAppInterface::ConsoleMessageLevel)lvl);
}



void ChaiScriptingPlugin::executionCalled(const std::string& evalStatement)
{
	try
	{
		chai->eval(evalStatement);
	}
	catch (const chaiscript::exception::eval_error & ee) 
	{
		m_app->dispToConsole(ee.what(), ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		if (ee.call_stack.size() > 0) 
		{
			m_app->dispToConsole(QString("during evaluation at (%1, %2)").arg(ee.call_stack[0]->start().line).arg(ee.call_stack[0]->start().column), ccMainAppInterface::WRN_CONSOLE_MESSAGE);
		}
		std::cout << std::endl;
	}
	catch (const std::exception & e) 
	{
		m_app->dispToConsole(e.what(), ccMainAppInterface::ERR_CONSOLE_MESSAGE);

	}
}

void ChaiScriptingPlugin::resetToDefaultChaiState()
{
	if (!chai)
	{
		setupChaiScriptEngine();
	}
	else
	{
		chai->set_state(chaiInitialState);
	}
}

void ChaiScriptingPlugin::saveChaiState()
{
	chaiSaveState = chai->get_state();
}

void ChaiScriptingPlugin::resetToSavedChaiState()
{
	if (!chai)
	{
		setupChaiScriptEngine();
	}
	else
	{
		chai->set_state(chaiSaveState);
	}
}

QList<QAction *> ChaiScriptingPlugin::getActions()
{
	if ( !m_action )
	{
		m_action = new QAction( getName(), this );
		m_action->setToolTip( getDescription() );
		m_action->setIcon( getIcon() );
		
		// Connect appropriate signal
		connect( m_action, &QAction::triggered, this, &ChaiScriptingPlugin::openScriptEditor);
	}

	return { m_action };
}

void ChaiScriptingPlugin::openScriptEditor()
{
	if (!cseMW)
	{
		cseMW = chaiScriptCodeEditorMainWindow::TheInstance();
	}
	if (!cseMW)
	{
		m_app->dispToConsole("Failed to open chaiScriptCodeEditorMainWindow", ccMainAppInterface::ERR_CONSOLE_MESSAGE);
		return;
	}
	cseMW->show();
}


ChaiScriptingPlugin::~ChaiScriptingPlugin()
{
	try
	{
		delete chai;
	}
	catch (...)
	{
			
	}
}