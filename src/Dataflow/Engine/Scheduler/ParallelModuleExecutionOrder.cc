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

#include <Dataflow/Engine/Scheduler/ParallelModuleExecutionOrder.h>
#include <boost/foreach.hpp>

using namespace SCIRun::Dataflow::Engine;
using namespace SCIRun::Dataflow::Networks;

ParallelModuleExecutionOrder::ParallelModuleExecutionOrder()
{
}

ParallelModuleExecutionOrder::ParallelModuleExecutionOrder(const ParallelModuleExecutionOrder& other) : map_(other.map_)
{
}

ParallelModuleExecutionOrder::ParallelModuleExecutionOrder(const ParallelModuleExecutionOrder::ModulesByGroup& map) : map_(map)
{
}

ParallelModuleExecutionOrder::const_iterator ParallelModuleExecutionOrder::begin() const
{
  return map_.begin();
}

ParallelModuleExecutionOrder::const_iterator ParallelModuleExecutionOrder::end() const
{
  return map_.end();
}

int ParallelModuleExecutionOrder::minGroup() const
{
  return map_.empty() ? -1 : map_.begin()->first;
}

int ParallelModuleExecutionOrder::maxGroup() const
{
  return map_.empty() ? -1 : map_.rbegin()->first;
}

std::pair<ParallelModuleExecutionOrder::const_iterator, ParallelModuleExecutionOrder::const_iterator> ParallelModuleExecutionOrder::getGroup(int order) const
{
  return map_.equal_range(order);
}

std::ostream& SCIRun::Dataflow::Engine::operator<<(std::ostream& out, const ParallelModuleExecutionOrder& order)
{
  auto range = std::make_pair(order.begin(), order.end());
  BOOST_FOREACH(const ParallelModuleExecutionOrder::ModulesByGroup::value_type& v, range)
  {
    out << v.first << " " << v.second << std::endl;
  }
  return out;
}