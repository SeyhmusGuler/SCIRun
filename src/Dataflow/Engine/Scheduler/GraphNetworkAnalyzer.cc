/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2012 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
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

#include <boost/utility.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/foreach.hpp>

#include <Dataflow/Network/NetworkInterface.h>
#include <Dataflow/Network/ConnectionId.h>
#include <Dataflow/Engine/Scheduler/GraphNetworkAnalyzer.h>

using namespace SCIRun::Dataflow::Engine;
using namespace SCIRun::Dataflow::Networks;

NetworkGraphAnalyzer::NetworkGraphAnalyzer(const NetworkInterface& network, const SCIRun::Dataflow::Networks::ModuleFilter& moduleFilter) : moduleCount_(0)
{
  for (int i = 0; i < network.nmodules(); ++i)
  {
    auto module = network.module(i);
    if (moduleFilter(module))
    {
      moduleIdLookup_.left.insert(std::make_pair(module->get_id(), moduleCount_));
      moduleCount_++;
    }
  }

  std::vector<Edge> edges;

  BOOST_FOREACH(const ConnectionDescription& cd, network.connections())
  {
    if (moduleIdLookup_.left.find(cd.out_.moduleId_) != moduleIdLookup_.left.end()
      && moduleIdLookup_.left.find(cd.in_.moduleId_) != moduleIdLookup_.left.end())
    {
      edges.push_back(std::make_pair(moduleIdLookup_.left.at(cd.out_.moduleId_), moduleIdLookup_.left.at(cd.in_.moduleId_)));
    }
  }

  graph_ = Graph(edges.begin(), edges.end(), moduleCount_);

  try
  {
    boost::topological_sort(graph_, std::front_inserter(order_));
  }
  catch (std::invalid_argument& e)
  {
    BOOST_THROW_EXCEPTION(NetworkHasCyclesException() << SCIRun::Core::ErrorMessage(e.what()));
  }
}

const ModuleId& NetworkGraphAnalyzer::moduleAt(int vertex) const
{
  return moduleIdLookup_.right.at(vertex);
}

NetworkGraphAnalyzer::ExecutionOrder::iterator NetworkGraphAnalyzer::topologicalBegin()
{
  return order_.begin();
}

NetworkGraphAnalyzer::ExecutionOrder::iterator NetworkGraphAnalyzer::topologicalEnd()
{
  return order_.end();
}

NetworkGraphAnalyzer::Graph& NetworkGraphAnalyzer::graph()
{
  return graph_;
}

int NetworkGraphAnalyzer::moduleCount() const 
{
  return moduleCount_;
}