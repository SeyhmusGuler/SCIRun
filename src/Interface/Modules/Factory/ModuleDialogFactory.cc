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

#include <Interface/Modules/Factory/ModuleDialogFactory.h>
#include <Interface/Modules/Base/ModuleDialogBasic.h>
#include <Interface/Modules/Testing/SendScalarDialog.h>
#include <Interface/Modules/Testing/ReceiveScalarDialog.h>
#include <Interface/Modules/DataIO/ReadMatrixDialog.h>
#include <Interface/Modules/DataIO/WriteMatrixDialog.h>
#include <Interface/Modules/DataIO/ReadMeshDialog.h>
#include <Interface/Modules/DataIO/ReadFieldDialog.h>
#include <Interface/Modules/DataIO/WriteFieldDialog.h>
#include <Interface/Modules/Math/EvaluateLinearAlgebraUnaryDialog.h>
#include <Interface/Modules/Math/EvaluateLinearAlgebraBinaryDialog.h>
#include <Interface/Modules/Math/ReportMatrixInfoDialog.h>
#include <Interface/Modules/Math/CreateMatrixDialog.h>
#include <Interface/Modules/Math/AppendMatrixDialog.h>
#include <Interface/Modules/Math/SolveLinearSystemDialog.h>
#include <Interface/Modules/String/CreateStringDialog.h>
#include <Interface/Modules/String/PrintDatatypeDialog.h>
#include <Interface/Modules/Fields/CreateLatVolDialog.h>
#include <Interface/Modules/Fields/GetDomainBoundaryDialog.h>
#include <Interface/Modules/Fields/ReportFieldInfoDialog.h>
#include <Interface/Modules/Fields/JoinFieldsDialog.h>
#include <Interface/Modules/Fields/SplitFieldByDomainDialog.h>
#include <Interface/Modules/FiniteElements/TDCSSimulatorDialog.h>
#include <Interface/Modules/BrainStimulator/SetConductivitiesToTetMeshDialog.h>
#include <Interface/Modules/BrainStimulator/ElectrodeCoilSetupDialog.h>
#include <Interface/Modules/BrainStimulator/GenerateROIStatisticsDialog.h>
#include <Interface/Modules/BrainStimulator/SetupRHSforTDCSandTMSDialog.h>
#include <Interface/Modules/Visualization/MatrixAsVectorFieldDialog.h>
#include <Interface/Modules/Visualization/ShowStringDialog.h>
#include <Interface/Modules/Visualization/ShowFieldDialog.h>
#include <Interface/Modules/Visualization/CreateBasicColorMapDialog.h>
#include <Interface/Modules/Render/ViewScene.h>
#include <Interface/Modules/Bundle/InsertFieldsIntoBundleDialog.h>
#include <Interface/Modules/Bundle/GetFieldsFromBundleDialog.h>

using namespace SCIRun::Gui;
using namespace SCIRun::Dataflow::Networks;

ModuleDialogFactory::ModuleDialogFactory(QWidget* parentToUse) : parentToUse_(parentToUse) {}

ModuleDialogGeneric* ModuleDialogFactory::makeDialog(const std::string& moduleId, ModuleStateHandle state)
{
  if (moduleId.find("SendScalar") != std::string::npos)
    return new SendScalarDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ReceiveScalar") != std::string::npos)
    return new ReceiveScalarDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ReadMatrix") != std::string::npos)
    return new ReadMatrixDialog(moduleId, state, parentToUse_);
  if (moduleId.find("WriteMatrix") != std::string::npos)
    return new WriteMatrixDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ReadMesh") != std::string::npos)
    return new ReadMeshDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ReadField") != std::string::npos)
    return new ReadFieldDialog(moduleId, state, parentToUse_);
  if (moduleId.find("WriteField") != std::string::npos)
    return new WriteFieldDialog(moduleId, state, parentToUse_);
  if (moduleId.find("EvaluateLinearAlgebraUnary") != std::string::npos)
    return new EvaluateLinearAlgebraUnaryDialog(moduleId, state, parentToUse_);
  if (moduleId.find("EvaluateLinearAlgebraBinary") != std::string::npos)
    return new EvaluateLinearAlgebraBinaryDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ShowString") != std::string::npos)
    return new ShowStringDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ShowField") != std::string::npos)
    return new ShowFieldDialog(moduleId, state, parentToUse_);
  if (moduleId.find("AppendMatrix") != std::string::npos)
    return new AppendMatrixDialog(moduleId, state, parentToUse_);
  if (moduleId.find("CreateMatrix") != std::string::npos)
    return new CreateMatrixDialog(moduleId, state, parentToUse_);
  if (moduleId.find("CreateString") != std::string::npos)
    return new CreateStringDialog(moduleId, state, parentToUse_);
  if (moduleId.find("PrintDatatype") != std::string::npos)
    return new PrintDatatypeDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ReportMatrixInfo") != std::string::npos)
    return new ReportMatrixInfoDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ReportFieldInfo") != std::string::npos)
    return new ReportFieldInfoDialog(moduleId, state, parentToUse_);
  if (moduleId.find("MatrixAsVectorField") != std::string::npos)
    return new MatrixAsVectorFieldDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ViewScene") != std::string::npos)
    return new ViewSceneDialog(moduleId, state, parentToUse_);
  if (moduleId.find("SolveLinearSystem") != std::string::npos)
    return new SolveLinearSystemDialog(moduleId, state, parentToUse_);
  if (moduleId.find("CreateLatVol") != std::string::npos)
    return new CreateLatVolDialog(moduleId, state, parentToUse_);
  if (moduleId.find("tDCSSimulator") != std::string::npos)
    return new TDCSSimulatorDialog(moduleId, state, parentToUse_);
  if (moduleId.find("ElectrodeCoilSetup") != std::string::npos)
    return new ElectrodeCoilSetupDialog(moduleId, state, parentToUse_);
  if (moduleId.find("SetConductivitiesToTetMesh") != std::string::npos)
    return new SetConductivitiesToTetMeshDialog(moduleId, state, parentToUse_); 
  if (moduleId.find("GenerateROIStatistics") != std::string::npos)
    return new GenerateROIStatisticsDialog(moduleId, state, parentToUse_); 
  if (moduleId.find("SetupRHSforTDCSandTMS") != std::string::npos)
    return new SetupRHSforTDCSandTMSDialog(moduleId, state, parentToUse_);       
  if (moduleId.find("CreateStandardColorMap") != std::string::npos)
    return new CreateBasicColorMapDialog(moduleId, state, parentToUse_);
  if (moduleId.find("GetDomainBoundary") != std::string::npos)
    return new GetDomainBoundaryDialog(moduleId, state, parentToUse_);
  if (moduleId.find("JoinFields") != std::string::npos)
    return new JoinFieldsDialog(moduleId, state, parentToUse_);
  if (moduleId.find("InsertFieldsIntoBundle") != std::string::npos)
    return new InsertFieldsIntoBundleDialog(moduleId, state, parentToUse_);
  if (moduleId.find("GetFieldsFromBundle") != std::string::npos)
    return new GetFieldsFromBundleDialog(moduleId, state, parentToUse_);
  if (moduleId.find("SplitFieldByDomain") != std::string::npos)
    return new SplitFieldByDomainDialog(moduleId, state, parentToUse_);
  else
    return new ModuleDialogBasic(moduleId, parentToUse_);
}
