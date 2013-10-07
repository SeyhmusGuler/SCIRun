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

#include <Dataflow/Network/Module.h>
#include <gtest/gtest.h>

using namespace SCIRun::Dataflow::Networks;

TEST(ModuleTests, CanBuildWithPorts)
{
  ModuleHandle module = Module::Builder().with_name("SolveLinearSystem")
    .add_input_port(Port::ConstructionParams("ForwardMatrix", "Matrix"))
    .add_input_port(Port::ConstructionParams("RHS", "Matrix"))
    .add_output_port(Port::ConstructionParams("Solution", "Matrix"))
    .build();
  EXPECT_EQ(2, module->num_input_ports());
  EXPECT_EQ(1, module->num_output_ports());
  EXPECT_EQ("SolveLinearSystem", module->get_module_name());
  EXPECT_EQ("SolveLinearSystem:0", module->get_id().id_);
}

TEST(ModuleIdTests, CanConstructFromString)
{
  ModuleId m1("ComputeSVD:5");
  EXPECT_EQ(5, m1.idNumber_);
  EXPECT_EQ("ComputeSVD:5", m1.id_);
  EXPECT_EQ("ComputeSVD", m1.name_);
}

TEST(ModuleIdTests, CanMatchMultiDigitIdNumber)
{
  ModuleId m1("ComputeSVD:12");
  EXPECT_EQ(12, m1.idNumber_);
  EXPECT_EQ("ComputeSVD:12", m1.id_);
  EXPECT_EQ("ComputeSVD", m1.name_);
}

TEST(ModuleIdTests, WhatHappensWhenNoNumberAtEnd)
{
  EXPECT_THROW(ModuleId("ComputeSVD"), SCIRun::Core::InvalidArgumentException);
}