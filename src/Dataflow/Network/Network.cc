/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
   University of Utah.

   
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/

#include <stdexcept>
#include <sstream>
#include <iostream>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <Dataflow/Network/Network.h>
#include <Dataflow/Network/Connection.h>
#include <Dataflow/Network/Module.h>
#include <Dataflow/Network/ModuleDescription.h>
#include <Dataflow/Network/ModuleFactory.h>

using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Core::Algorithms;

Network::Network(ModuleFactoryHandle moduleFactory, ModuleStateFactoryHandle stateFactory, AlgorithmFactoryHandle algoFactory)
  : moduleFactory_(moduleFactory), stateFactory_(stateFactory), errorCode_(0)
{
  moduleFactory_->setStateFactory(stateFactory_);
  moduleFactory_->setAlgorithmFactory(algoFactory);
}

Network::~Network()
{
}

ModuleHandle Network::add_module(const ModuleLookupInfo& info)
{
  ModuleHandle module = moduleFactory_->create(moduleFactory_->lookupDescription(info));
  modules_.push_back(module);
  if (module)
  {
    module->connectErrorListener(boost::bind(&NetworkInterface::incrementErrorCode, this, _1));
  }
  return module;
}

bool Network::remove_module(const ModuleId& id)
{
  Modules::iterator loc = std::find_if(modules_.begin(), modules_.end(), boost::lambda::bind(&ModuleInterface::get_id, *boost::lambda::_1) == id);
  if (loc != modules_.end())
  {
    // Inform the module that it is about to be erased from the network...
    modules_.erase(loc);
    return true;
  }
  return false;
}

ConnectionId Network::connect(const ConnectionOutputPort& out, const ConnectionInputPort& in)
{
  ModuleHandle outputModule = out.first;
  ModuleHandle inputModule = in.first;
  size_t outputPortIndex = out.second;
  size_t inputPortIndex = in.second;

  ENSURE_NOT_NULL(outputModule, "cannot connect null output module");
  ENSURE_NOT_NULL(inputModule, "cannot connect null input module");
  
  // assure that the ports are not altered while connecting
  //m1->oports_.lock();
  //m2->iports_.lock();

  // dynamic port safeguard.
  //if (m2->lastportdynamic_ && p2 >= m2->iports_.size()) {
  //  p2 = m2->iports_.size() - 1;
  //}

  if (outputPortIndex >= outputModule->num_output_ports() || inputPortIndex >= inputModule->num_input_ports())
  {
    //TODO: log
    //std::cout << "TODO: ERROR OR NOT?: connection not available, ports do not exist!" << std::endl;
    return ConnectionId("");
  }

  ConnectionId id = ConnectionId::create(ConnectionDescription(
    OutgoingConnectionDescription(outputModule->get_id(), outputPortIndex), 
    IncomingConnectionDescription(inputModule->get_id(), inputPortIndex)));
  if (connections_.find(id) == connections_.end())
  {
    try
    {
      ConnectionHandle conn(boost::make_shared<Connection>(outputModule, outputPortIndex, inputModule, inputPortIndex, id));

      //lock.lock();

      connections_[id] = conn;

      // Reschedule next time we can.
      //reschedule=1;

      //lock.unlock();

      //m1->oports_.unlock();
      //m2->iports_.unlock();

      return id;
    }
    catch (const std::invalid_argument& e)
    {
      std::cout << "Caught exception making a connection: " << e.what() << std::endl;
      ///????????
      return ConnectionId(""); //??
    }
  }
  return ConnectionId(""); //??
}

bool Network::disconnect(const ConnectionId& id)
{
  Connections::iterator loc = connections_.find(id);
  if (loc != connections_.end())
  {
    connections_.erase(loc);
    return true;
  }
  return false;
}

void Network::disable_connection(const ConnectionId&)
{
  //TODO
}

size_t Network::nmodules() const
{
  return modules_.size();
}

ModuleHandle Network::module(size_t i) const
{
  if (i >= nmodules())
    THROW_OUT_OF_RANGE("Module index out of range");

  return modules_[i];
}

ModuleHandle Network::lookupModule(const ModuleId& id) const
{
  Modules::const_iterator i = std::find_if(modules_.begin(), modules_.end(), boost::lambda::bind(&ModuleInterface::get_id, *boost::lambda::_1) == id);
  return i == modules_.end() ? ModuleHandle() : *i;
}

ExecutableObject* Network::lookupExecutable(const ModuleId& id) const
{
  return lookupModule(id).get();
}

size_t Network::nconnections() const
{
  return connections_.size();
}

struct GetConnectionIds
{
  std::string operator()(const Network::Connections::value_type& pair) const
  {
    return pair.first.id_;
  }
};

std::string Network::toString() const
{
  using boost::lambda::bind;
  std::ostringstream ostr;
  ostr << "~~~NETWORK DESCRIPTION~~~\n";
  ostr << "Modules:\n";
  std::transform(modules_.begin(), modules_.end(), std::ostream_iterator<std::string>(ostr, ", "), [](ModuleHandle m) { return to_string(*m);});
  ostr << "\nConnections:\n";
  std::transform(connections_.begin(), connections_.end(), std::ostream_iterator<std::string>(ostr, ", "), GetConnectionIds());
  return ostr.str();
}

struct Describe
{
  ConnectionDescription operator()(const Network::Connections::value_type& p) const
  {
    return p.first.describe();
  }
};

NetworkInterface::ConnectionDescriptionList Network::connections() const
{
  ConnectionDescriptionList conns;
  std::transform(connections_.begin(), connections_.end(), std::back_inserter(conns), Describe());
  return conns;
}

int Network::errorCode() const
{
  return errorCode_;
}

void Network::incrementErrorCode(const ModuleId& moduleId)
{
  errorCode_++;
  //TODO: store errored modules in a list or something
}

NetworkGlobalSettings& Network::settings()
{
  return settings_;
}
