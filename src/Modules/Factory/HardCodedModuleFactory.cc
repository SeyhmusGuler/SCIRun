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

#include <iostream>
#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <boost/functional/factory.hpp>
#include <Modules/Factory/HardCodedModuleFactory.h>
#include <Dataflow/Network/ModuleDescription.h>
#include <Dataflow/Network/Module.h>

#include <Modules/Basic/ReceiveScalar.h>
#include <Modules/Basic/SendScalar.h>
#include <Modules/Basic/ReceiveTestMatrix.h>
#include <Modules/Basic/SendTestMatrix.h>
#include <Modules/Basic/PrintDatatype.h>
#include <Modules/Math/EvaluateLinearAlgebraUnary.h>
#include <Modules/Math/EvaluateLinearAlgebraBinary.h>
#include <Modules/Math/ReportMatrixInfo.h>
#include <Modules/Math/AppendMatrix.h>
#include <Modules/Math/CreateMatrix.h>
#include <Modules/Math/SolveLinearSystem.h>
#include <Modules/Fields/CreateLatVolMesh.h>
#include <Modules/Fields/CreateScalarFieldDataBasic.h>
#include <Modules/Fields/ReportFieldInfo.h>
#include <Modules/Legacy/Fields/CreateLatVol.h>
#include <Modules/Fields/FieldToMesh.h>
#include <Modules/DataIO/ReadMatrix.h>
#include <Modules/DataIO/WriteMatrix.h>
#include <Modules/DataIO/ReadMesh.h>
#include <Modules/DataIO/ReadField.h>
#include <Modules/DataIO/WriteField.h>
#include <Modules/String/CreateString.h>
#include <Modules/Visualization/ShowString.h>
#include <Modules/Visualization/ShowField.h>
#include <Modules/Visualization/MatrixAsVectorField.h>
#include <Modules/Render/ViewScene.h>

#include <Dataflow/Network/Tests/SimpleSourceSink.h>
#include <Modules/Factory/Share.h>

using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::Modules::Factory;
using namespace SCIRun::Modules::Basic;
using namespace SCIRun::Modules::Math;
using namespace SCIRun::Modules::Fields;
using namespace SCIRun::Modules::DataIO;
using namespace SCIRun::Modules::StringProcessing;
using namespace SCIRun::Modules::Visualization;
using namespace SCIRun::Modules::Render;
using namespace boost::assign;

HardCodedModuleFactory::HardCodedModuleFactory() : renderer_(0)
{
  Module::Builder::use_sink_type(boost::factory<SimpleSink*>());
  Module::Builder::use_source_type(boost::factory<SimpleSource*>());
}

void HardCodedModuleFactory::setStateFactory(SCIRun::Dataflow::Networks::ModuleStateFactoryHandle stateFactory)
{
  stateFactory_ = stateFactory;
  Module::defaultStateFactory_ = stateFactory_;
}

void HardCodedModuleFactory::setRenderer(SCIRun::Dataflow::Networks::RendererInterface* renderer)
{
  renderer_ = renderer;
}

ModuleHandle HardCodedModuleFactory::create(const ModuleDescription& desc)
{
  Module::Builder builder;
  
  if (desc.maker_)
    builder.using_func(desc.maker_);
  else
    builder.with_name(desc.lookupInfo_.module_name_);

  BOOST_FOREACH(const InputPortDescription& input, desc.input_ports_)
  {
    builder.add_input_port(Port::ConstructionParams(input.name, input.datatype, input.color));
  }
  BOOST_FOREACH(const OutputPortDescription& output, desc.output_ports_)
  {
    builder.add_output_port(Port::ConstructionParams(output.name, output.datatype, output.color));
  }

  if (desc.lookupInfo_.module_name_.find("ComputeSVD") != std::string::npos)
    builder.disable_ui();

  ModuleHandle module = builder.build();

  if (desc.lookupInfo_.module_name_.find("ViewScene") != std::string::npos)
    boost::dynamic_pointer_cast<ViewScene>(module)->setRenderer(renderer_);

  return module;
}

//TODO: make more generic...macros?

ModuleDescription HardCodedModuleFactory::lookupDescription(const ModuleLookupInfo& info)
{
  std::string name = info.module_name_;
  ModuleDescription description;
  description.lookupInfo_ = info;
  if (name.find("ComputeSVD") != std::string::npos)
  {
    description.input_ports_ += InputPortDescription("Input", "Matrix", "blue");
    description.output_ports_ += OutputPortDescription("U", "Matrix", "blue"), OutputPortDescription("S", "Matrix", "blue"), OutputPortDescription("V", "Matrix", "blue");
    description.maker_ = 0;
  }
  else if (name.find("ReadMatrix") != std::string::npos)
  {
    description.input_ports_ += ReadMatrixModule::inputPortDescription(ReadMatrixModule::inputPort0Name());
    description.output_ports_ = ReadMatrixModule::outputPortDescription(ReadMatrixModule::outputPort0Name(), ReadMatrixModule::outputPort1Name());
    description.maker_ = boost::factory<ReadMatrixModule*>();
  }
  else if (name.find("WriteMatrix") != std::string::npos)
  {
    description.input_ports_ = WriteMatrixModule::inputPortDescription(WriteMatrixModule::inputPort0Name(), WriteMatrixModule::inputPort1Name());
    description.maker_ = boost::factory<WriteMatrixModule*>();
  }
  else if (name.find("ReadField") != std::string::npos)
  {
    description.input_ports_ += ReadFieldModule::inputPortDescription(ReadFieldModule::inputPort0Name());
    description.output_ports_ = ReadFieldModule::outputPortDescription(ReadFieldModule::outputPort0Name(), ReadFieldModule::outputPort1Name());
    description.maker_ = boost::factory<ReadFieldModule*>();
  }
  else if (name.find("WriteField") != std::string::npos)
  {
    description.input_ports_ = WriteFieldModule::inputPortDescription(WriteFieldModule::inputPort0Name(), WriteFieldModule::inputPort1Name());
    description.maker_ = boost::factory<WriteFieldModule*>();
  }
  else if (name.find("ReadMesh") != std::string::npos)
  {
    description.input_ports_ += ReadMeshModule::inputPortDescription(ReadMeshModule::inputPort0Name());
    description.output_ports_ = ReadMeshModule::outputPortDescription(ReadMeshModule::outputPort0Name(), ReadMeshModule::outputPort1Name());
    description.maker_ = boost::factory<ReadMeshModule*>();
  }
  else if (name.find("SendScalar") != std::string::npos)
  {
    description.output_ports_ += SendScalarModule::outputPortDescription(SendScalarModule::outputPort0Name());
    description.maker_ = boost::factory<SendScalarModule*>();
  }
  else if (name.find("ReceiveScalar") != std::string::npos)
  {
    description.input_ports_ += ReceiveScalarModule::inputPortDescription(ReceiveScalarModule::inputPort0Name());
    description.maker_ = boost::factory<ReceiveScalarModule*>();
  }
  else if (name.find("PrintDatatype") != std::string::npos)
  {
    description.input_ports_ += PrintDatatypeModule::inputPortDescription(PrintDatatypeModule::inputPort0Name());
    description.maker_ = boost::factory<PrintDatatypeModule*>();
  }
  else if (name.find("SendTestMatrix") != std::string::npos)
  {
    description.output_ports_ += SendTestMatrixModule::outputPortDescription(SendTestMatrixModule::outputPort0Name());
    description.maker_ = boost::factory<SendTestMatrixModule*>();
  }
  else if (name.find("ReceiveTestMatrix") != std::string::npos)
  {
    description.input_ports_ += ReceiveTestMatrixModule::inputPortDescription(ReceiveTestMatrixModule::inputPort0Name());
    description.maker_ = boost::factory<ReceiveTestMatrixModule*>();
  }
  else if (name.find("ReportMatrixInfo") != std::string::npos)
  {
    description.input_ports_ += ReportMatrixInfoModule::inputPortDescription(ReportMatrixInfoModule::inputPort0Name());
    description.maker_ = boost::factory<ReportMatrixInfoModule*>();
  }
  else if (name.find("ReportFieldInfo") != std::string::npos)
  {
    description.input_ports_ += ReportFieldInfoModule::inputPortDescription(ReportFieldInfoModule::inputPort0Name());
    description.output_ports_ = ReportFieldInfoModule::outputPortDescription(ReportFieldInfoModule::outputPort0Name(), ReportFieldInfoModule::outputPort1Name());
    description.maker_ = boost::factory<ReportFieldInfoModule*>();
  }
  else if (name.find("AppendMatrix") != std::string::npos)
  {
    description.input_ports_ = AppendMatrixModule::inputPortDescription(AppendMatrixModule::inputPort0Name(), AppendMatrixModule::inputPort1Name());
    description.output_ports_ += AppendMatrixModule::outputPortDescription(AppendMatrixModule::outputPort0Name());
    description.maker_ = boost::factory<AppendMatrixModule*>();
  }
  else if (name.find("EvaluateLinearAlgebraUnary") != std::string::npos)
  {
    description.input_ports_ += EvaluateLinearAlgebraUnaryModule::inputPortDescription(EvaluateLinearAlgebraUnaryModule::inputPort0Name());
    description.output_ports_ += EvaluateLinearAlgebraUnaryModule::outputPortDescription(EvaluateLinearAlgebraUnaryModule::outputPort0Name());
    description.maker_ = boost::factory<EvaluateLinearAlgebraUnaryModule*>();
  }
  else if (name.find("EvaluateLinearAlgebraBinary") != std::string::npos)
  {
    description.input_ports_ = EvaluateLinearAlgebraBinaryModule::inputPortDescription(EvaluateLinearAlgebraBinaryModule::inputPort0Name(), EvaluateLinearAlgebraBinaryModule::inputPort1Name());
    description.output_ports_ += EvaluateLinearAlgebraBinaryModule::outputPortDescription(EvaluateLinearAlgebraBinaryModule::outputPort0Name());
    description.maker_ = boost::factory<EvaluateLinearAlgebraBinaryModule*>();
  }
  else if (name.find("CreateMatrix") != std::string::npos)
  {
    description.output_ports_ += CreateMatrixModule::outputPortDescription(CreateMatrixModule::outputPort0Name());
    description.maker_ = boost::factory<CreateMatrixModule*>();
  }
  else if (name.find("SolveLinearSystem") != std::string::npos)
  {
    description.input_ports_ = SolveLinearSystemModule::inputPortDescription(SolveLinearSystemModule::inputPort0Name(), SolveLinearSystemModule::inputPort1Name());
    description.output_ports_ += SolveLinearSystemModule::outputPortDescription(SolveLinearSystemModule::outputPort0Name());
    description.maker_ = boost::factory<SolveLinearSystemModule*>();
  }
  else if (name.find("CreateString") != std::string::npos)
  {
    description.output_ports_ += CreateStringModule::outputPortDescription(CreateStringModule::outputPort0Name());
    description.maker_ = boost::factory<CreateStringModule*>();
  }
  else if (name.find("ShowString") != std::string::npos)
  {
    description.input_ports_ += ShowStringModule::inputPortDescription(ShowStringModule::inputPort0Name());
    description.output_ports_ += ShowStringModule::outputPortDescription(ShowStringModule::outputPort0Name());
    description.maker_ = boost::factory<ShowStringModule*>();
  }
  else if (name.find("ShowField") != std::string::npos)
  {
    description.input_ports_ += ShowFieldModule::inputPortDescription(ShowFieldModule::inputPort0Name());
    description.output_ports_ += ShowFieldModule::outputPortDescription(ShowFieldModule::outputPort0Name());
    description.maker_ = boost::factory<ShowFieldModule*>();
  }
  else if (name.find("CreateLatVolMesh") != std::string::npos)
  {
    description.output_ports_ += CreateLatVolMesh::outputPortDescription(CreateLatVolMesh::outputPort0Name());
    description.maker_ = boost::factory<CreateLatVolMesh*>();
  }
  else if (name.find("MatrixAsVectorField") != std::string::npos)
  {
    description.input_ports_ += MatrixAsVectorFieldModule::inputPortDescription(MatrixAsVectorFieldModule::inputPort0Name());
    description.output_ports_ += MatrixAsVectorFieldModule::outputPortDescription(MatrixAsVectorFieldModule::outputPort0Name());
    description.maker_ = boost::factory<MatrixAsVectorFieldModule*>();
  }
  else if (name.find("CreateLatVol") != std::string::npos)
  {
    description.input_ports_ = CreateLatVol::inputPortDescription(CreateLatVol::inputPort0Name(), CreateLatVol::inputPort1Name());
    description.output_ports_ += CreateLatVol::outputPortDescription(CreateLatVol::outputPort0Name());
    description.maker_ = boost::factory<CreateLatVol*>();
  }
  else if (name.find("FieldToMesh") != std::string::npos)
  {
    description.input_ports_ += FieldToMesh::inputPortDescription(FieldToMesh::inputPort0Name());
    description.output_ports_ += FieldToMesh::outputPortDescription(FieldToMesh::outputPort0Name());
    description.maker_ = boost::factory<FieldToMesh*>();
  }
  else if (name.find("CreateScalarFieldDataBasic") != std::string::npos)
  {
    description.input_ports_ += CreateScalarFieldDataBasic::inputPortDescription(CreateScalarFieldDataBasic::inputPort0Name());
    description.output_ports_ += CreateScalarFieldDataBasic::outputPortDescription(CreateScalarFieldDataBasic::outputPort0Name());
    description.maker_ = boost::factory<CreateScalarFieldDataBasic*>();
  }
  else if (name.find("ViewScene") != std::string::npos)
  {
    description.input_ports_ = ViewScene::inputPortDescription(ViewScene::inputPort0Name(), ViewScene::inputPort1Name());
    description.maker_ = boost::factory<ViewScene*>();
  }
  else
  {
    //TODO: log
    std::ostringstream ostr;
    ostr << "Error: Undefined module \"" << name << "\"";
    THROW_INVALID_ARGUMENT(ostr.str());
  }
  return description;
}

