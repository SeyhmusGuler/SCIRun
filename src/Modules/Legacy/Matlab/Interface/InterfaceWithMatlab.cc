/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2015 Scientific Computing and Imaging Institute,
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

/*
 *  InterfaceWithMatlab.cc:
 *
 *  Written by:
 *   Jeroen Stinstra
 *
 */

#include <Modules/Legacy/Matlab/Interface/InterfaceWithMatlab.h>
#include <Core/Logging/Log.h>

#include <Core/Matlab/matlabconverter.h>
#include <Core/Matlab/matlabfile.h>
#include <Core/Matlab/matlabarray.h>

#include <Core/Thread/ConditionVariable.h>

#include <Core/SystemCall/TempFileManager.h>
#include <Core/Utils/Legacy/soloader.h>

#include <Core/Services/ServiceClient.h>
#include <Core/ICom/IComPacket.h>
#include <Core/ICom/IComAddress.h>
#include <Modules/Legacy/Matlab/Interface/Services/MatlabEngine.h>
#include <boost/thread.hpp>
#include <Core/Services/FileTransferClient.h>
#include <Core/Datatypes/Matrix.h>
#include <Core/Datatypes/String.h>
#include <Core/Datatypes/Legacy/Field/Field.h>
#include <Core/Thread/Legacy/CleanupManager.h>

#if 0

#include <Core/Datatypes/MatrixTypeConverter.h>

#include <Core/Thread/Runnable.h>

#include <Core/Services/Service.h>
#include <Core/Services/ServiceBase.h>

#include <Core/ICom/IComSocket.h>

#endif

#include <iostream>
#include <fstream>

using namespace SCIRun;
using namespace SCIRun::Modules::Matlab::Interface;
using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Core::Algorithms;
using namespace SCIRun::Core::Algorithms::Matlab;
using namespace SCIRun::Dataflow::Networks;
using namespace SCIRun::MatlabIO;
using namespace SCIRun::Core::Logging;
using namespace SCIRun::Core::Thread;

//TODO: temporary for compilation.
#ifdef _WIN32
#define USE_MATLAB_ENGINE_LIBRARY
#endif

#ifdef USE_MATLAB_ENGINE_LIBRARY
// symbols from the matlab engine library - import from the dll/so manually to not
// explicitly require matlab (it won't work without it, but will compile and SCIRun will run)
typedef struct engine Engine;	/* Incomplete definition for Engine */
typedef Engine* (*ENGOPENPROC)(const char*);
typedef int(*ENGCLOSEPROC)(Engine*);
typedef int(*ENGSETVISIBLEPROC)(Engine*, bool);
typedef int(*ENGEVALSTRINGPROC)(Engine*, const char*);
typedef int(*ENGOUTPUTBUFFERPROC)(Engine*, char*, int);

ENGOPENPROC engOpen = 0;
ENGCLOSEPROC engClose = 0;
ENGSETVISIBLEPROC engSetVisible = 0;
ENGEVALSTRINGPROC engEvalString = 0;
ENGOUTPUTBUFFERPROC engOutputBuffer = 0;
#endif

namespace MatlabImpl
{
  class InterfaceWithMatlabEngineThreadInfo;
  class InterfaceWithMatlabEngineThread;

  typedef boost::shared_ptr<InterfaceWithMatlabEngineThreadInfo> InterfaceWithMatlabEngineThreadInfoHandle;

  class InterfaceWithMatlabEngineThread : /*public Runnable,*/ public ServiceBase
  {
  public:
    InterfaceWithMatlabEngineThread(ServiceClientHandle serv_handle, InterfaceWithMatlabEngineThreadInfoHandle info_handle);
    void operator()();

  private:
    ServiceClientHandle serv_handle_;
    InterfaceWithMatlabEngineThreadInfoHandle info_handle_;
  };

  class InterfaceWithMatlabEngineThreadInfo
  {
  public:
    InterfaceWithMatlabEngineThreadInfo();

    void dolock();
    void unlock();

    std::string         output_cmd_;

    ConditionVariable   wait_code_done_;
    bool                code_done_;
    bool                code_success_;
    std::string         code_error_;

    ConditionVariable   wait_exit_;
    bool                exit_;
    bool                passed_test_;
    Mutex lock_;

  };
}

namespace SCIRun {
  namespace Modules {
    namespace Matlab {
      namespace Interface {

        enum //TODO: keep this in sync with number of output ports
        {
          NUM_OUTPUT_MATRICES = 2,
          NUM_OUTPUT_FIELDS = 2,
          NUM_OUTPUT_NRRDS = 0,
          NUM_OUTPUT_STRINGS = 2
        };

        class InterfaceWithMatlabImpl : public ServiceBase
        {
        public:
          explicit InterfaceWithMatlabImpl(InterfaceWithMatlab* module);
          ~InterfaceWithMatlabImpl();

          static matlabarray::mitype	convertdataformat(const std::string& dataformat);
          static std::string totclstring(const std::string& instring);
          std::vector<std::string> converttcllist(const Variable::List& str);

          void	update_status(const std::string& text);

          bool	open_matlab_engine();
          bool	close_matlab_engine();

          bool	create_temp_directory();
          bool	delete_temp_directory();

          bool	save_input_matrices();
          bool	load_output_matrices();

          bool	generate_matlab_code();
          bool	send_matlab_job();
          bool  send_input(const std::string& str);

          bool	synchronise_input();

        private:
          InterfaceWithMatlab* module_;
          // Temp directory for writing files coming from the
          // the matlab engine

          boost::filesystem::path temp_directory_;

          //TODO: need a better solution for this one
          bool isMatrixOutputPortConnected(int index) const;
          bool isFieldOutputPortConnected(int index) const;
          bool isStringOutputPortConnected(int index) const;
          void sendMatrixOutput(int index, MatrixHandle matrix) const;
          void sendFieldOutput(int index, FieldHandle field) const;
          void sendStringOutput(int index, StringHandle str) const;

          // GUI variables
#if 0
          // Names of matrices
          GuiString   input_matrix_name_;
          GuiString   input_field_name_;
          GuiString   input_nrrd_name_;
          GuiString   input_string_name_;
          GuiString   input_matrix_type_;
          GuiString   input_nrrd_type_;
          GuiString   input_matrix_array_;
          GuiString   input_field_array_;
          GuiString   input_nrrd_array_;
          GuiString   output_matrix_name_;
          GuiString   output_field_name_;
          GuiString   output_nrrd_name_;
          GuiString   output_string_name_;
          GuiString   configfile_;

#endif
          // Fields per port
          std::vector<std::string>   input_matrix_name_list_;
          std::vector<std::string>   input_matrix_name_list_old_;
          std::vector<std::string>   input_field_name_list_;
          std::vector<std::string>   input_field_name_list_old_;
          std::vector<std::string>   input_nrrd_name_list_;
          std::vector<std::string>   input_string_name_list_old_;
          std::vector<std::string>   input_string_name_list_;
          std::vector<std::string>   input_nrrd_name_list_old_;
          std::vector<std::string>   input_matrix_type_list_;
          std::vector<std::string>   input_nrrd_type_list_;
          std::vector<std::string>   input_matrix_array_list_;
          std::vector<std::string>   input_field_array_list_;
          std::vector<std::string>   input_nrrd_array_list_;
          std::vector<std::string>   input_string_array_list_;
          std::vector<std::string>   input_matrix_type_list_old_;
          std::vector<std::string>   input_nrrd_type_list_old_;
          std::vector<std::string>   input_matrix_array_list_old_;
          std::vector<std::string>   input_field_array_list_old_;
          std::vector<std::string>   input_nrrd_array_list_old_;
          std::vector<std::string>   output_matrix_name_list_;
          std::vector<std::string>   output_field_name_list_;
          std::vector<std::string>   output_nrrd_name_list_;
          std::vector<std::string>   output_string_name_list_;

          std::vector<int> input_matrix_generation_old_;
          std::vector<int> input_field_generation_old_;
          std::vector<int> input_nrrd_generation_old_;
          std::vector<int> input_string_generation_old_;

          std::string	matlab_code_list_;

          // Ports for input and output
          std::vector<std::string>		input_matrix_matfile_;
          std::vector<std::string>		input_field_matfile_;
          std::vector<std::string>		input_nrrd_matfile_;
          std::vector<std::string>		input_string_matfile_;

          std::vector<std::string>		output_matrix_matfile_;
          std::vector<std::string>		output_field_matfile_;
          std::vector<std::string>		output_nrrd_matfile_;
          std::vector<std::string>		output_string_matfile_;
#if 0
          // Internet connectivity stuff
          GuiString   inet_address_;
          GuiString   inet_port_;
          GuiString   inet_passwd_;
          GuiString   inet_session_;

          std::string inet_address_old_;
          std::string inet_port_old_;
          std::string inet_passwd_old_;
          std::string inet_session_old_;
#endif
          // The tempfilemanager
          TempFileManager tfmanager_;
          std::string		mfile_;
#if 0
          GuiString   matlab_code_file_;
          GuiString		matlab_var_;

          GuiString   start_matlab_;
          GuiInt      matlab_timeout_;

          std::string start_matlab_old_;
          int         matlab_timeout_old_;
#endif

#ifndef USE_MATLAB_ENGINE_LIBRARY
          ServiceClientHandle           matlab_engine_;
          MatlabImpl::InterfaceWithMatlabEngineThreadInfoHandle	thread_info_;
#else
          Engine* engine_;
          char output_buffer_[51200];
#endif

          FileTransferClientHandle      file_transfer_;

          bool            need_file_transfer_;
          boost::filesystem::path remote_tempdir_;
          std::string     inputstring_;

        public:
          static void cleanup_callback(void *data);
        };
      }
    }
  }
}

MatlabImpl::InterfaceWithMatlabEngineThreadInfo::InterfaceWithMatlabEngineThreadInfo() :
wait_code_done_("InterfaceWithMatlabEngineInfo condition variable code"),
code_done_(false),
code_success_(false),
wait_exit_("InterfaceWithMatlabEngineInfo condition variable exit"),
exit_(false),
passed_test_(false),
lock_("matlabThreadLock")
{
}

void MatlabImpl::InterfaceWithMatlabEngineThreadInfo::dolock()
{
  lock_.lock();
}

void MatlabImpl::InterfaceWithMatlabEngineThreadInfo::unlock()
{
  lock_.unlock();
}

MatlabImpl::InterfaceWithMatlabEngineThread::InterfaceWithMatlabEngineThread(ServiceClientHandle serv_handle, InterfaceWithMatlabEngineThreadInfoHandle info_handle) :
serv_handle_(serv_handle),
info_handle_(info_handle)
{
}

void MatlabImpl::InterfaceWithMatlabEngineThread::operator()()
{
  IComPacketHandle	packet;
  bool				done = false;
  while (!done)
  {
    if (!(serv_handle_->recv(packet)))
    {
      info_handle_->dolock();
      if (info_handle_->exit_ == true)
      {
        // It crashed as result of closing of connection
        // Anyway, the module was destroyed so it should not
        // matter anymore
        info_handle_->wait_code_done_.conditionBroadcast();
        info_handle_->wait_exit_.conditionBroadcast();
        info_handle_->unlock();
        return;
      }
      info_handle_->code_done_ = true;
      info_handle_->code_success_ = false;
      info_handle_->wait_code_done_.conditionBroadcast();
      info_handle_->exit_ = true;
      info_handle_->wait_exit_.conditionBroadcast();
      info_handle_->unlock();

      done = true;
      continue;
    }

    info_handle_->dolock();

    if (info_handle_->exit_ == true)
    {
      info_handle_->wait_exit_.conditionBroadcast();
      info_handle_->unlock();
      return;
    }

    switch (packet->gettag())
    {
    case TAG_STDO:
    {
      std::string str;
      if (packet->getparam1() < 0) str = "STDOUT END";
      else str = packet->getstring();
#if 0
      std::string cmd = info_handle_->output_cmd_ + " \"" + InterfaceWithMatlab::totclstring(str) + "\"";
      info_handle_->unlock();
      TCLInterface::lock();
      TCLInterface::execute(cmd);
      TCLInterface::unlock();
#endif
    }
    break;
    case TAG_STDE:
    {
      std::string str;
      if (packet->getparam1() < 0) str = "STDERR END";
      else str = packet->getstring();
#if 0
      std::string cmd = info_handle_->output_cmd_ + " \"STDERR: " + InterfaceWithMatlab::totclstring(str) + "\"";
      info_handle_->unlock();
      TCLInterface::lock();
      TCLInterface::execute(cmd);
      TCLInterface::unlock();
#endif
    }
    break;
    case TAG_END_:
    case TAG_EXIT:
    {
      info_handle_->code_done_ = true;
      info_handle_->code_success_ = false;
      info_handle_->wait_code_done_.conditionBroadcast();
      info_handle_->exit_ = true;
      info_handle_->wait_exit_.conditionBroadcast();
      done = true;
      info_handle_->unlock();
    }
    break;
    case TAG_MCODE_SUCCESS:
    {
      info_handle_->code_done_ = true;
      info_handle_->code_success_ = true;
      info_handle_->wait_code_done_.conditionBroadcast();
      info_handle_->unlock();
    }
    break;
    case TAG_MCODE_ERROR:
    {
      info_handle_->code_done_ = true;
      info_handle_->code_success_ = false;
      info_handle_->code_error_ = packet->getstring();
      info_handle_->wait_code_done_.conditionBroadcast();
      info_handle_->unlock();
    }
    break;
    default:
      info_handle_->unlock();
    }
  }
}

#if 0
DECLARE_MAKER(InterfaceWithMatlab)

InterfaceWithMatlab::InterfaceWithMatlab(GuiContext *context) :
Module("InterfaceWithMatlab", context, Filter, "Interface", "MatlabInterface"),
input_matrix_name_(context->subVar("input-matrix-name")),
input_field_name_(context->subVar("input-field-name")),
input_nrrd_name_(context->subVar("input-nrrd-name")),
input_string_name_(context->subVar("input-string-name")),
input_matrix_type_(context->subVar("input-matrix-type")),
input_nrrd_type_(context->subVar("input-nrrd-type")),
input_matrix_array_(context->subVar("input-matrix-array")),
input_field_array_(context->subVar("input-field-array")),
input_nrrd_array_(context->subVar("input-nrrd-array")),
output_matrix_name_(context->subVar("output-matrix-name")),
output_field_name_(context->subVar("output-field-name")),
output_nrrd_name_(context->subVar("output-nrrd-name")),
output_string_name_(context->subVar("output-string-name")),
configfile_(context->subVar("configfile")),
inet_address_(context->subVar("inet-address")),
inet_port_(context->subVar("inet-port")),
inet_passwd_(context->subVar("inet-passwd")),
inet_session_(context->subVar("inet-session")),
matlab_code_(context->subVar("matlab-code")),
matlab_code_file_(context->subVar("matlab-code-file")),
matlab_var_(context->subVar("matlab-var")),
start_matlab_(context->subVar("start-matlab")),
matlab_timeout_(context->subVar("matlab-timeout")),
matlab_timeout_old_(180),
{
#ifdef USE_MATLAB_ENGINE_LIBRARY
  engine_ = 0;
#endif
  // find the input and output ports

  input_matrix_name_list_.resize(NUM_MATRIX_PORTS);
  input_matrix_name_list_old_.resize(NUM_MATRIX_PORTS);
  input_field_name_list_.resize(NUM_FIELD_PORTS);
  input_field_name_list_old_.resize(NUM_FIELD_PORTS);
  input_nrrd_name_list_.resize(NUM_NRRD_PORTS);
  input_nrrd_name_list_old_.resize(NUM_NRRD_PORTS);
  input_string_name_list_.resize(NUM_STRING_PORTS);
  input_string_name_list_old_.resize(NUM_STRING_PORTS);

  input_matrix_array_list_old_.resize(NUM_MATRIX_PORTS);
  input_field_array_list_old_.resize(NUM_FIELD_PORTS);
  input_nrrd_array_list_old_.resize(NUM_NRRD_PORTS);

  input_matrix_type_list_old_.resize(NUM_MATRIX_PORTS);
  input_nrrd_type_list_old_.resize(NUM_NRRD_PORTS);

  input_matrix_generation_old_.resize(NUM_MATRIX_PORTS);
  for (int p = 0; p<NUM_MATRIX_PORTS; p++)  input_matrix_generation_old_[p] = -1;
  input_field_generation_old_.resize(NUM_FIELD_PORTS);
  for (int p = 0; p<NUM_FIELD_PORTS; p++)  input_field_generation_old_[p] = -1;
  input_nrrd_generation_old_.resize(NUM_NRRD_PORTS);
  for (int p = 0; p<NUM_NRRD_PORTS; p++)  input_nrrd_generation_old_[p] = -1;
  input_string_generation_old_.resize(NUM_STRING_PORTS);
  for (int p = 0; p<NUM_STRING_PORTS; p++)  input_string_generation_old_[p] = -1;
}

#endif

InterfaceWithMatlabImpl::InterfaceWithMatlabImpl(InterfaceWithMatlab* module) :
module_(module),
//matlab_timeout_old_(180),
output_matrix_matfile_(NUM_OUTPUT_MATRICES),
output_field_matfile_(NUM_OUTPUT_FIELDS),
output_nrrd_matfile_(NUM_OUTPUT_NRRDS),
output_string_matfile_(NUM_OUTPUT_STRINGS),
#ifdef USE_MATLAB_ENGINE_LIBRARY
engine_(0),
#endif
need_file_transfer_(false)
{
  CleanupManager::add_callback(InterfaceWithMatlabImpl::cleanup_callback, reinterpret_cast<void*>(this));
}

// Function for cleaning up
// matlab modules
void InterfaceWithMatlabImpl::cleanup_callback(void* data)
{
  InterfaceWithMatlabImpl* ptr = reinterpret_cast<InterfaceWithMatlabImpl*>(data);
  // We just want to make sure that the matlab engine is released and
  // any temp dirs are cleaned up
  ptr->close_matlab_engine();
  ptr->delete_temp_directory();
}

InterfaceWithMatlabImpl::~InterfaceWithMatlabImpl()
{
  // Again if we registered a module for destruction and we are removing it
  // we need to unregister
  CleanupManager::invoke_remove_callback(InterfaceWithMatlabImpl::cleanup_callback, reinterpret_cast<void*>(this));
}


void InterfaceWithMatlabImpl::update_status(const std::string& text)
{
  LOG_DEBUG(module_->get_id().id_ << " UpdateStatus \"" << text << "\"");
}

matlabarray::mitype InterfaceWithMatlabImpl::convertdataformat(const std::string& dataformat)
{
  matlabarray::mitype type = matlabarray::miUNKNOWN;
  if (dataformat == "same as data")  { type = matlabarray::miSAMEASDATA; }
  else if (dataformat == "double")   { type = matlabarray::miDOUBLE; }
  else if (dataformat == "single")   { type = matlabarray::miSINGLE; }
  else if (dataformat == "uint64")   { type = matlabarray::miUINT64; }
  else if (dataformat == "int64")    { type = matlabarray::miINT64; }
  else if (dataformat == "uint32")   { type = matlabarray::miUINT32; }
  else if (dataformat == "int32")    { type = matlabarray::miINT32; }
  else if (dataformat == "uint16")   { type = matlabarray::miUINT16; }
  else if (dataformat == "int16")    { type = matlabarray::miINT16; }
  else if (dataformat == "uint8")    { type = matlabarray::miUINT8; }
  else if (dataformat == "int8")     { type = matlabarray::miINT8; }
  return (type);
}

// converttcllist:
// converts a TCL formatted list into a STL array
// of strings

std::vector<std::string> InterfaceWithMatlabImpl::converttcllist(const Variable::List& vars)
{
  std::vector<std::string> list;
  std::transform(vars.begin(), vars.end(), std::back_inserter(list), [](const Variable& v) { return v.toString(); });
  return list;
}

bool InterfaceWithMatlabImpl::synchronise_input()
{
  auto state = module_->get_state();
  input_matrix_name_list_ = converttcllist(state->getValue(Parameters::InputMatrixNames).toVector());
  input_matrix_type_list_ = converttcllist(state->getValue(Parameters::InputMatrixTypes).toVector());
  input_matrix_array_list_ = converttcllist(state->getValue(Parameters::InputMatrixArrays).toVector());
  output_matrix_name_list_ = converttcllist(state->getValue(Parameters::OutputMatrixNames).toVector());
#if 0
  str = input_field_name_.get(); input_field_name_list_ = converttcllist(str);
  str = input_field_array_.get(); input_field_array_list_ = converttcllist(str);
  str = output_field_name_.get(); output_field_name_list_ = converttcllist(str);

  str = input_nrrd_name_.get(); input_nrrd_name_list_ = converttcllist(str);
  str = input_nrrd_type_.get(); input_nrrd_type_list_ = converttcllist(str);
  str = input_nrrd_array_.get(); input_nrrd_array_list_ = converttcllist(str);
  str = output_nrrd_name_.get(); output_nrrd_name_list_ = converttcllist(str);

  str = input_string_name_.get(); input_string_name_list_ = converttcllist(str);
  str = output_string_name_.get(); output_string_name_list_ = converttcllist(str);
#endif
  matlab_code_list_ = state->getValue(Parameters::MatlabCode).toString();

  return(true);
}


const ModuleLookupInfo InterfaceWithMatlab::staticInfo_("InterfaceWithMatlab", "Interface", "Matlab");

InterfaceWithMatlab::InterfaceWithMatlab() : Module(staticInfo_), impl_(new InterfaceWithMatlabImpl(this))
{
  INITIALIZE_PORT(InputMatrix);
  INITIALIZE_PORT(InputField);
  INITIALIZE_PORT(InputString);
  INITIALIZE_PORT(OutputField0);
  INITIALIZE_PORT(OutputMatrix0);
  INITIALIZE_PORT(OutputString0);
  INITIALIZE_PORT(OutputField1);
  INITIALIZE_PORT(OutputMatrix1);
  INITIALIZE_PORT(OutputString1);
}

void InterfaceWithMatlab::setStateDefaults()
{

}

void InterfaceWithMatlab::execute()
{
  // Synchronise input: translate TCL lists into C++ STL lists
  if (!impl_->synchronise_input())
  {
    error("InterfaceWithMatlab: Could not retreive GUI input");
    return;
  }

  // If we haven't created a temporary directory yet
  // we open one to store all temp files in
  if (!impl_->create_temp_directory())
  {
    error("InterfaceWithMatlab: Could not create temporary directory");
    return;
  }

  if (!impl_->open_matlab_engine())
  {
    error("InterfaceWithMatlab: Could not open matlab engine");
    return;
  }

  if (!impl_->save_input_matrices())
  {
    error("InterfaceWithMatlab: Could not create the input matrices");
    return;
  }

  update_state(Executing);

  if (!impl_->generate_matlab_code())
  {
    error("InterfaceWithMatlab: Could not create m-file code for matlabengine");
    return;
  }

  if (!impl_->send_matlab_job())
  {
    error("InterfaceWithMatlab: Matlab returned an error or Matlab could not be launched");
    return;
  }

  if (!impl_->load_output_matrices())
  {
    error("InterfaceWithMatlab: Could not load matrices that matlab generated");
    return;
  }
}

bool InterfaceWithMatlabImpl::send_matlab_job()
{
  std::string mfilename = mfile_.substr(0, mfile_.size() - 2); // strip the .m
  auto remotefile = file_transfer_->remote_file(mfilename);
#ifndef USE_MATLAB_ENGINE_LIBRARY
  IComPacketHandle packet(new IComPacket);

  if (!packet)
  {
    module_->error("InterfaceWithMatlab: Could not create packet");
    return(false);
  }

  thread_info_->dolock();
  thread_info_->code_done_ = false;
  thread_info_->unlock();

  packet->settag(TAG_MCODE);
  packet->setstring(remotefile.string());
  matlab_engine_->send(packet);

  thread_info_->dolock();
  if (!thread_info_->code_done_)
  {
    UniqueLock lock(thread_info_->lock_.get());
    thread_info_->wait_code_done_.wait(lock);
  }
  bool success = thread_info_->code_success_;
  bool exitcond = thread_info_->exit_;
  if (!success)
  {
    if (exitcond)
    {
      std::ostringstream ostr;
      ostr <<
        "InterfaceWithMatlab: the matlab engine crashed or did not start: " << thread_info_->code_error_ << "\n" <<
        "Possible causes are:\n" <<
        "(1) matlab code failed in such a way that the engine was no able to catch the error (e.g. failure mex of code)\n" <<
        "(2) matlab crashed and the matlab engine detected an end of communication of the matlab process\n" <<
        "(3) temparory files could not be created or are corrupted\n" <<
        "(4) improper matlab version, use matlab V5 or higher, currently matlab V5-V7 are supported";
      module_->error(ostr.str());
    }
    else
    {
      std::ostringstream ostr;
      ostr <<
        "InterfaceWithMatlab: matlab code failed: " << thread_info_->code_error_ << "\n"
        "InterfaceWithMatlab: Detected an error in the Matlab code, the matlab engine is still running and caught the exception\n" <<
        "InterfaceWithMatlab: Please check the matlab code in the GUI and try again. The output window in the GUI should contain the reported error message generated by matlab";
      module_->error(ostr.str());
    }
    thread_info_->code_done_ = false;
    thread_info_->unlock();
    return(false);
  }
  thread_info_->code_done_ = false;
  thread_info_->unlock();

#else
  std::string command = std::string("addpath('") + remotefile.string().substr(0, remotefile.string().size() - 12) + "');";
  bool success = (engEvalString(engine_, command.c_str()) == 0);

  command = "scirun_code;";
  success = (engEvalString(engine_, command.c_str()) == 0);

  std::string output(output_buffer_);
  std::string cmd = module_->get_id().id_ + " AddOutput \"" + totclstring(output) + "\"";
#ifdef SCIRUN4_CODE_TO_BE_ENABLED_LATER
  TCLInterface::execute(cmd);
#endif

  std::cout << output_buffer_ << std::endl;

  // without this, matlab will cache the first file as a function and ignore the rest of them.
  engEvalString(engine_, "clear functions");
  // output_buffer_ has output in it - do something with it
#endif
  return(success);
}

#if 0
bool InterfaceWithMatlab::send_input(std::string str)
{
#ifndef USE_MATLAB_ENGINE_LIBRARY
  IComPacketHandle packet = new IComPacket;

  if (matlab_engine_.get_rep() == 0) return(true);

  if (packet.get_rep() == 0)
  {
    error("InterfaceWithMatlab: Could not create packet");
    return(false);
  }

  packet->settag(TAG_INPUT);
  packet->setstring(str);
  matlab_engine_->send(packet);

  return(true);
#else
  if (engine_ == 0) return true;
  engEvalString(engine_, str.c_str());
  return true;
#endif
}
#endif

bool InterfaceWithMatlabImpl::open_matlab_engine()
{
  std::string matlablibrary;
  const std::string inetaddress, inetport, passwd, session;
  std::string startmatlab = module_->get_state()->getValue(Parameters::MatlabPath).toString();
#if 0
  inetaddress = inet_address_.get();
  inetport = inet_port_.get();
  passwd = inet_passwd_.get();
  session = inet_session_.get();

  if (startmatlab.empty())
    if ( sci_getenv("SCIRUN_STARTMATLAB") )
      startmatlab = sci_getenv("SCIRUN_STARTMATLAB");

  if ( sci_getenv("SCIRUN_MATLABLIBRARY") ) matlablibrary = sci_getenv("SCIRUN_MATLABLIBRARY");
#endif
  int timeout = 60;
#if 0
  std::string timeout_str = sci_getenv("SCIRUN_MATLABTIMEOUT");
  from_string(timeout_str,timeout);

  // Check whether someone touched the engine settings
  // if so close the engine and record the current settings
  if ((timeout != matlab_timeout_old_)||(startmatlab != start_matlab_old_)||(inetaddress != inet_address_old_)||(inetport != inet_port_old_)||(passwd != inet_passwd_old_)||(session != inet_session_old_))
  {
    close_matlab_engine();
    inet_address_old_ = inetaddress;
    inet_port_old_ = inetport;
    inet_passwd_old_ = passwd;
    inet_session_old_ = session;
    matlab_timeout_old_ = timeout;
    start_matlab_old_ = startmatlab;
  }
#endif
#ifndef USE_MATLAB_ENGINE_LIBRARY
  if (!matlab_engine_)
#else
  if (!engine_)
#endif
  {
    IComAddressHandle address(new IComAddress);
    if (inetaddress.empty())
    {
      address->setaddress("internal", "servicemanager");
    }
    else
    {
      address->setaddress("scirun", inetaddress, inetport);
    }

    int sessionnum;
    try
    {
      sessionnum = boost::lexical_cast<int>(session);
    }
    catch (boost::bad_lexical_cast&)
    {
      sessionnum = 1;
    }

    update_status("Please wait while launching matlab, this may take a few minutes ....\n");

#ifndef USE_MATLAB_ENGINE_LIBRARY
    matlab_engine_.reset(new ServiceClient());
    if(!(matlab_engine_->open(address,"matlabengine",sessionnum,passwd,timeout,startmatlab)))
    {
      std::ostringstream ostr;
      ostr << "InterfaceWithMatlab: Could not open matlab engine (error=" << matlab_engine_->geterror() << ").\n" <<
        "Make sure the matlab engine has not been disabled in [SCIRUN_DIRECTORY]/services/matlabengine.rc\n" <<
        "Press the 'Edit Local Config of Matlab Engine' to change the configuration.\n" <<
        "Check remote address information, or leave all fields except 'session' blank to connect to local matlab engine.";
      module_->error(ostr.str());
      matlab_engine_.reset();
      return(false);
    }
#else
    if (engOpen == 0)
    {
      // load functions from dll
      std::string error_msg;

      LIBRARY_HANDLE englib = 0;
      if (matlablibrary.size() > 10)
      {
        size_t size = matlablibrary.size();
        if (matlablibrary.substr(size - 10) == "libeng.dll")
        {
          englib = GetLibraryHandle(matlablibrary.c_str(), error_msg);
        }
      }

      if (!englib)
      {
        englib = GetLibraryHandle("libeng.dll", error_msg);
      }

      if (!englib)
      {
        module_->error(std::string("Could not open matlab library libeng."));
        return false;
      }

      engOpen = (ENGOPENPROC)GetHandleSymbolAddress(englib, "engOpen", error_msg);
      engClose = (ENGCLOSEPROC)GetHandleSymbolAddress(englib, "engClose", error_msg);
      engSetVisible = (ENGSETVISIBLEPROC)GetHandleSymbolAddress(englib, "engSetVisible", error_msg);
      engEvalString = (ENGEVALSTRINGPROC)GetHandleSymbolAddress(englib, "engEvalString", error_msg);
      engOutputBuffer = (ENGOUTPUTBUFFERPROC)GetHandleSymbolAddress(englib, "engOutputBuffer", error_msg);

      if (!engOpen || !engClose || !engSetVisible || !engEvalString || !engOutputBuffer)
      {
        if (!engOpen) module_->error("Cannot find engOpen");
        if (!engClose) module_->error("Cannot find engClose");
        if (!engSetVisible) module_->error("Cannot find engSetVisible");
        if (!engEvalString) module_->error("Cannot find engEvalString");
        if (!engOutputBuffer) module_->error("Cannot find engOutputBuffer");
        module_->error("Could not open matlab engine functions from matlab library");
        return false;
      }
    }


    if (inetaddress == "")
    {
      engine_ = engOpen(NULL);

      if (!engine_)
      {
        system("matlab /regserver");
        engine_ = engOpen(NULL);
      }
    }
    else
    {
      engine_ = engOpen(inetaddress.c_str());
    }

    if (!engine_)
    {
      module_->error("InterfaceWithMatlab: Could not open matlab engine. Check remote address information, or leave all fields except 'session' blank to connect to local matlab engine.");
      return false;
    }

    engOutputBuffer(engine_, output_buffer_, 51200);
    engSetVisible(engine_, true);
#endif


    file_transfer_.reset(new FileTransferClient());
    if (!file_transfer_->open(address, "matlabenginefiletransfer", sessionnum, passwd))
    {
      std::string err;
#ifndef USE_MATLAB_ENGINE_LIBRARY
      err = matlab_engine_->geterror();
      matlab_engine_->close();
      matlab_engine_.reset();
#else
      engClose(engine_);
      engine_ = 0;
#endif
      std::ostringstream ostr;
      ostr << "InterfaceWithMatlab: Could not open matlab engine file transfer service (error=" << err << ")" <<
        ".\n  Make sure the matlab engine file transfer service has not been disabled in [MATLAB_DIRECTPRY]/services/matlabengine.rc.\n" <<
        " Press the 'Edit Local Config of Matlab Engine' to change the configuration." <<
        " Check remote address information, or leave all fields except 'session' blank to connect to local matlab engine.";
      module_->error(ostr.str());
      file_transfer_.reset();
      return(false);
    }

    need_file_transfer_ = false;
    std::string localid;
    std::string remoteid;
    file_transfer_->get_local_homedirid(localid);
    file_transfer_->get_remote_homedirid(remoteid);

    if (localid != remoteid)
    {
      need_file_transfer_ = true;
      if (!(file_transfer_->create_remote_tempdir("matlab-engine.XXXXXX", remote_tempdir_)))
      {
#ifndef USE_MATLAB_ENGINE_LIBRARY
        matlab_engine_->close();
        matlab_engine_.reset();
#else
        engClose(engine_);
        engine_ = 0;
#endif
        module_->error("InterfaceWithMatlab: Could not create remote temporary directory.");
        file_transfer_->close();
        file_transfer_.reset();
        return(false);
      }
      file_transfer_->set_local_dir(temp_directory_);
      file_transfer_->set_remote_dir(remote_tempdir_);
    }
    else
    {
      // Although they might share a home directory
      // This directory can be mounted at different trees
      // Hence we translate between both. InterfaceWithMatlab does not like
      // the use of $HOME
      file_transfer_->set_local_dir(temp_directory_);
      auto tempdir = temp_directory_;
      file_transfer_->translate_scirun_tempdir(tempdir);
      file_transfer_->set_remote_dir(tempdir);
    }

#ifndef USE_MATLAB_ENGINE_LIBRARY
    IComPacketHandle packet;
    if(!(matlab_engine_->recv(packet)))
    {
      matlab_engine_->close();
      file_transfer_->close();
      std::ostringstream ostr;
      ostr << "Could not get answer from matlab engine (error=" << matlab_engine_->geterror() << ").\n " <<
        " This is an internal communication error, make sure that the portnumber is correct. \n" <<
        " If address information is correct, this most probably points to a bug in the SCIRun software.";
      module_->error(ostr.str());

      matlab_engine_.reset();
      file_transfer_.reset();

      return(false);
    }


    if (packet->gettag() == TAG_MERROR)
    {
      matlab_engine_->close();
      file_transfer_->close();

      std::ostringstream ostr;
      ostr << "InterfaceWithMatlab engine returned an error (error=" << packet->getstring() << ")" <<
        "\n Please check whether '[MATLAB_DIRECTORY]/services/matlabengine.rc' has been setup properly." <<
        "\n Press the 'Edit Local Config of Matlab Engine' to change the configuration." <<
        "\n Edit the 'startmatlab=' line to start matlab properly. " <<
        "\n If you running matlab remotely, this file must be edited on the machine running matlab.";
      module_->error(ostr.str());

      matlab_engine_.reset();
      file_transfer_.reset();

      return(false);
    }

    thread_info_.reset(new MatlabImpl::InterfaceWithMatlabEngineThreadInfo());
    if (!thread_info_)
    {
      matlab_engine_->close();
      file_transfer_->close();

      module_->error("InterfaceWithMatlab: Could not create thread information object");
      matlab_engine_.reset();
      file_transfer_.reset();

      return(false);
    }

    thread_info_->output_cmd_ = module_->get_id().id_ + " AddOutput";

    // By cloning the object, it will have the same fields and sockets, but the socket
    // and error handling will be separate. As the thread will invoke its own instructions
    // it is better to have a separate copy. Besides, the socket obejct will point to the
    // same underlying socket. Hence only the error handling part will be duplicated

    ServiceClientHandle matlab_engine_copy(matlab_engine_->clone());
    MatlabImpl::InterfaceWithMatlabEngineThread enginethread(matlab_engine_copy,thread_info_);

    boost::thread thread(enginethread);
    thread.detach();

    int sessionn = packet->getparam1();
    matlab_engine_->setsession(sessionn);

    std::string sharehomedir = "yes";
    if (need_file_transfer_) sharehomedir = "no";

    std::ostringstream statusStr;
    statusStr << "Matlab engine running\n\nmatlabengine version: " << matlab_engine_->getversion()
      << "\nmatlabengine address: " << matlab_engine_->getremoteaddress()
      << "\nmatlabengine session: " + matlab_engine_->getsession()
      << "\nmatlabengine filetransfer version :" << file_transfer_->getversion()
      << "\nshared home directory: " << sharehomedir
      << "\nlocal temp directory: " << file_transfer_->local_file("")
      << "\nremote temp directory: " << file_transfer_->remote_file("") << "\n";
    auto status = statusStr.str();
#else
    std::string status = "InterfaceWithMatlab engine running\n";
#endif
    update_status(status);
  }

  return(true);
}


bool InterfaceWithMatlabImpl::close_matlab_engine()
{
#ifndef USE_MATLAB_ENGINE_LIBRARY
  if (matlab_engine_)
  {
    matlab_engine_->close();
    matlab_engine_.reset();
  }
  if (file_transfer_)
  {
    file_transfer_->close();
    file_transfer_.reset();
  }

  if (thread_info_)
  {
    thread_info_->dolock();
    if (!thread_info_->exit_)
    {
      thread_info_->exit_ = true;
      thread_info_->wait_exit_.conditionBroadcast();
    }
    thread_info_->unlock();
    thread_info_.reset();
  }
#else
  if (engine_)
  {
    engClose(engine_);
    engine_ = 0;
  }
#endif

  return(true);
}

bool InterfaceWithMatlabImpl::load_output_matrices()
{
  for (int p = 0; p < NUM_OUTPUT_MATRICES; p++)
  {
    if (!isMatrixOutputPortConnected(p))
      continue;
    if (output_matrix_name_list_[p].empty())
      continue;
    if (output_matrix_matfile_[p].empty())
      continue;

    matlabfile mf;
    matlabarray ma;

    try
    {
      if (need_file_transfer_) file_transfer_->get_file(file_transfer_->remote_file(output_matrix_matfile_[p]), file_transfer_->local_file(output_matrix_matfile_[p]));
      mf.open(file_transfer_->local_file(output_matrix_matfile_[p]).string(), "r");
      ma = mf.getmatlabarray(output_matrix_name_list_[p]);
      mf.close();
    }
    catch (...)
    {
      module_->error("InterfaceWithMatlab: Could not read output matrix");
      continue;
    }

    if (ma.isempty())
    {
      module_->error("InterfaceWithMatlab: Could not read output matrix");
      continue;
    }

    MatrixHandle handle;
    std::string info;
    matlabconverter translate(module_->getLogger());
    if (translate.sciMatrixCompatible(ma, info))
      translate.mlArrayTOsciMatrix(ma, handle);
    sendMatrixOutput(p, handle);
  }

#ifdef SCIRUN4_CODE_TO_BE_ENABLED_LATER
  for (int p = 0; p < NUM_FIELD_PORTS; p++)
  {
    if (!oport_connected(port)) { port++; continue; }

    // Test whether the field port exists
    if (output_field_name_list_[p] == "") continue;
    if (output_field_matfile_[p] == "") continue;

    matlabfile mf;
    matlabarray ma;
    try
    {
      if (need_file_transfer_) file_transfer_->get_file(file_transfer_->remote_file(output_field_matfile_[p]),file_transfer_->local_file(output_field_matfile_[p]));

      mf.open(file_transfer_->local_file(output_field_matfile_[p]),"r");
      ma = mf.getmatlabarray(output_field_name_list_[p]);
      mf.close();
    }
    catch(...)
    {
      error("InterfaceWithMatlab: Could not read output matrix");
      continue;
    }

    if (ma.isempty())
    {
      error("InterfaceWithMatlab: Could not read output matrix");
      continue;
    }

    FieldHandle handle;
    std::string info;
    matlabconverter translate(dynamic_cast<SCIRun::ProgressReporter *>(this));
    if (translate.sciFieldCompatible(ma,info)) translate.mlArrayTOsciField(ma,handle);
    send_output_handle(port,handle,true); port++;
  }


  for (int p = 0; p < NUM_NRRD_PORTS; p++)
  {
    if (!oport_connected(port)) { port++; continue; }

    // Test whether the nrrd port exists
    if (output_nrrd_name_list_[p] == "") continue;
    if (output_nrrd_matfile_[p] == "") continue;

    matlabfile mf;
    matlabarray ma;
    try
    {
      if (need_file_transfer_) file_transfer_->get_file(file_transfer_->remote_file(output_nrrd_matfile_[p]),file_transfer_->local_file(output_nrrd_matfile_[p]));
      mf.open(file_transfer_->local_file(output_nrrd_matfile_[p]),"r");
      ma = mf.getmatlabarray(output_nrrd_name_list_[p]);
      mf.close();
    }
    catch(...)
    {
      error("InterfaceWithMatlab: Could not read output matrix");
      continue;
    }

    if (ma.isempty())
    {
      error("InterfaceWithMatlab: Could not read output matrix");
      continue;
    }

    NrrdDataHandle handle;
    std::string info;
    matlabconverter translate(dynamic_cast<SCIRun::ProgressReporter *>(this));
    if (translate.sciNrrdDataCompatible(ma,info)) translate.mlArrayTOsciNrrdData(ma,handle);
    send_output_handle(port,handle,true); port++;
  }


  for (int p = 0; p < NUM_STRING_PORTS; p++)
  {
    if (!oport_connected(port)) { port++; continue; }

    // Test whether the nrrd port exists
    if (output_string_name_list_[p] == "") continue;
    if (output_string_matfile_[p] == "") continue;

    matlabfile mf;
    matlabarray ma;
    try
    {
      if (need_file_transfer_) file_transfer_->get_file(file_transfer_->remote_file(output_string_matfile_[p]),file_transfer_->local_file(output_string_matfile_[p]));
      mf.open(file_transfer_->local_file(output_string_matfile_[p]),"r");
      ma = mf.getmatlabarray(output_string_name_list_[p]);
      mf.close();
    }
    catch(...)
    {
      error("InterfaceWithMatlab: Could not read output matrix");
      continue;
    }

    if (ma.isempty())
    {
      error("InterfaceWithMatlab: Could not read output matrix");
      continue;
    }

    StringHandle handle;
    std::string info;
    matlabconverter translate(dynamic_cast<SCIRun::ProgressReporter *>(this));
    if (translate.sciStringCompatible(ma,info)) translate.mlArrayTOsciString(ma,handle);
    send_output_handle(port,handle,true); port++;
  }
#endif

  return(true);
}

bool InterfaceWithMatlabImpl::generate_matlab_code()
{
  std::ofstream m_file;

  mfile_ = "scirun_code.m";
  auto filename = file_transfer_->local_file(mfile_);
  m_file.open(filename.string(), std::ios::app);

  m_file << matlab_code_list_ << "\n";

  for (int p = 0; p < NUM_OUTPUT_MATRICES; p++)
  {
    if (!isMatrixOutputPortConnected(p))
      continue;

    if (output_matrix_name_list_[p].empty())
      continue;

    std::ostringstream oss;
    oss << "output_matrix" << p << ".mat";
    output_matrix_matfile_[p] = oss.str();
    std::string cmd;
    cmd = "if exist('" + output_matrix_name_list_[p] + "','var'), save " + file_transfer_->remote_file(output_matrix_matfile_[p]).string() + " " + output_matrix_name_list_[p] + "; end\n";
    m_file << cmd;
  }
  for (int p = 0; p < NUM_OUTPUT_FIELDS; p++)
  {
    if (!isFieldOutputPortConnected(p))
      continue;

    if (output_field_name_list_[p] == "") continue;

    std::ostringstream oss;
    oss << "output_field" << p << ".mat";
    output_field_matfile_[p] = oss.str();
    std::string cmd;
    cmd = "if exist('" + output_field_name_list_[p] + "','var'), save " + file_transfer_->remote_file(output_field_matfile_[p]).string() + " " + output_field_name_list_[p] + "; end\n";
    m_file << cmd;
  }
#ifdef SCIRUN4_CODE_TO_BE_ENABLED_LATER
  for (int p = 0; p < NUM_NRRD_PORTS; p++)
  {
    // Test whether the matrix port exists
    if (!oport_connected(port)) { port++; continue; }
    port++;
    if (output_nrrd_name_list_[p] == "") continue;

    std::ostringstream oss;

    oss << "output_nrrd" << p << ".mat";
    output_nrrd_matfile_[p] = oss.str();
    std::string cmd;
    cmd = "if exist('" + output_nrrd_name_list_[p] + "','var'), save " + file_transfer_->remote_file(output_nrrd_matfile_[p]) + " " + output_nrrd_name_list_[p] + "; end\n";
    m_file << cmd;
  }
#endif
  for (int p = 0; p < NUM_OUTPUT_STRINGS; p++)
  {
    if (!isStringOutputPortConnected(p))
      continue;

    if (output_string_name_list_[p] == "") continue;

    std::ostringstream oss;

    oss << "output_string" << p << ".mat";
    output_string_matfile_[p] = oss.str();
    std::string cmd;
    cmd = "if exist('" + output_string_name_list_[p] + "','var'), save " + file_transfer_->remote_file(output_string_matfile_[p]).string() + " " + output_string_name_list_[p] + "; end\n";
    m_file << cmd;
  }

  m_file.close();

  if (need_file_transfer_)
    file_transfer_->put_file(file_transfer_->local_file(mfile_), file_transfer_->remote_file(mfile_));

  return(true);
}

bool InterfaceWithMatlabImpl::isMatrixOutputPortConnected(int index) const
{
  switch (index)
  {
  case 0:
    return module_->oport_connected(module_->OutputMatrix0);
  case 1:
    return module_->oport_connected(module_->OutputMatrix1);
  default:
    return false;
  }
}

bool InterfaceWithMatlabImpl::isFieldOutputPortConnected(int index) const
{
  switch (index)
  {
  case 0:
    return module_->oport_connected(module_->OutputField0);
  case 1:
    return module_->oport_connected(module_->OutputField1);
  default:
    return false;
  }
}

bool InterfaceWithMatlabImpl::isStringOutputPortConnected(int index) const
{
  switch (index)
  {
  case 0:
    return module_->oport_connected(module_->OutputString0);
  case 1:
    return module_->oport_connected(module_->OutputString1);
  default:
    return false;
  }
}

void InterfaceWithMatlabImpl::sendMatrixOutput(int index, MatrixHandle matrix) const
{
  switch (index)
  {
  case 0:
    module_->sendOutput(module_->OutputMatrix0, matrix);
  case 1:
    module_->sendOutput(module_->OutputMatrix1, matrix);
  }
}

void InterfaceWithMatlabImpl::sendFieldOutput(int index, FieldHandle field) const
{
  switch (index)
  {
  case 0:
    module_->sendOutput(module_->OutputField0, field);
  case 1:
    module_->sendOutput(module_->OutputField1, field);
  }
}

void InterfaceWithMatlabImpl::sendStringOutput(int index, StringHandle str) const
{
  switch (index)
  {
  case 0:
    module_->sendOutput(module_->OutputString0, str);
  case 1:
    module_->sendOutput(module_->OutputString1, str);
  }
}

bool InterfaceWithMatlabImpl::save_input_matrices()
{
  try
  {
    int port = 0;

    std::ofstream m_file;
    std::string loadcmd;

    mfile_ = "scirun_code.m";
    auto filename = file_transfer_->local_file(mfile_);

    m_file.open(filename.string(), std::ios::out);

    auto matrices = module_->getRequiredDynamicInputs(module_->InputMatrix);
    input_matrix_matfile_.clear();
    input_matrix_name_list_.resize(matrices.size());
    input_matrix_type_list_.resize(matrices.size());
    input_matrix_array_list_.resize(matrices.size());
    input_matrix_name_list_old_.resize(matrices.size());
    input_matrix_type_list_old_.resize(matrices.size());
    input_matrix_array_list_old_.resize(matrices.size());
    input_matrix_generation_old_.resize(matrices.size());

    for (const auto& matrix : matrices)
    {
      // if the data as the same before
      // do nothing
      if ((input_matrix_name_list_[port] == input_matrix_name_list_old_[port]) &&
        (input_matrix_type_list_[port] == input_matrix_type_list_old_[port]) &&
        (input_matrix_array_list_[port] == input_matrix_array_list_old_[port]) &&
        (matrix->id() == input_matrix_generation_old_[port]))
      {
        // this one was not created again
        // hence we do not need to translate it again
        // with big datasets this should improve performance
        loadcmd = "load " + file_transfer_->remote_file(input_matrix_matfile_[port]).string() + ";\n";
        m_file << loadcmd;
        continue;
      }

      // Create a new filename for the input matrix
      std::ostringstream oss;
      oss << "input_matrix" << port << ".mat";
      input_matrix_matfile_.push_back(oss.str());

      matlabfile mf;
      matlabarray ma;

      mf.open(file_transfer_->local_file(input_matrix_matfile_[port]).string(), "w");
      mf.setheadertext("InterfaceWithMatlab V5 compatible file generated by SCIRun [module InterfaceWithMatlab version 1.3]");

      matlabconverter translate(module_->getLogger());
      translate.converttostructmatrix();
      if (input_matrix_array_list_[port] == "numeric array")
        translate.converttonumericmatrix();
      translate.setdatatype(convertdataformat(input_matrix_type_list_[port]));
      translate.sciMatrixTOmlArray(matrix, ma);

      mf.putmatlabarray(ma, input_matrix_name_list_[port]);
      mf.close();

      loadcmd = "load " + file_transfer_->remote_file(input_matrix_matfile_[port]).string() + ";\n";
      m_file << loadcmd;

      if (need_file_transfer_)
      {
        if (!(file_transfer_->put_file(
          file_transfer_->local_file(input_matrix_matfile_[port]),
          file_transfer_->remote_file(input_matrix_matfile_[port]))))
        {
          module_->error("InterfaceWithMatlab: Could not transfer file");
          std::string err = "Error :" + file_transfer_->geterror();
          module_->error(err);
          return(false);
        }
      }

      input_matrix_type_list_old_[port] = input_matrix_type_list_[port];
      input_matrix_array_list_old_[port] = input_matrix_array_list_[port];
      input_matrix_name_list_old_[port] = input_matrix_name_list_[port];
      input_matrix_generation_old_[port] = matrix->id();

      port++;
    }
#if 0
    for (int p = 0; p < NUM_FIELD_PORTS; p++)
    {
      FieldHandle	handle;
      if(!(get_input_handle(port,handle,false))) { port++; continue; }
      port++;

      // if there is no data
      if (handle.get_rep() == 0)
      {
        // we do not need the old file any more so delete it
        input_field_matfile_[p] = "";
        continue;
      }
      // if the data as the same before
      // do nothing
      if ((input_field_name_list_[p]==input_field_name_list_old_[p])&&
        (input_field_array_list_[p]==input_field_array_list_old_[p])&&
        (handle->generation == input_field_generation_old_[p]))
      {
        // this one was not created again
        // hence we do not need to translate it again
        // with big datasets this should improve performance
        loadcmd = "load " + file_transfer_->remote_file(input_field_matfile_[p]) + ";\n";
        m_file << loadcmd;

        continue;
      }

      // Create a new filename for the input matrix
      std::ostringstream oss;
      oss << "input_field" << p << ".mat";
      input_field_matfile_[p] = oss.str();

      matlabfile mf;
      matlabarray ma;

      mf.open(file_transfer_->local_file(input_field_matfile_[p]),"w");
      mf.setheadertext("InterfaceWithMatlab V5 compatible file generated by SCIRun [module InterfaceWithMatlab version 1.3]");

      matlabconverter translate(dynamic_cast<SCIRun::ProgressReporter *>(this));
      translate.converttostructmatrix();

      if (input_field_array_list_[p] == "numeric array")
      {
        translate.converttonumericmatrix();
      }
      translate.sciFieldTOmlArray(handle,ma);

      mf.putmatlabarray(ma,input_field_name_list_[p]);
      mf.close();

      loadcmd = "load " + file_transfer_->remote_file(input_field_matfile_[p]) + ";\n";
      m_file << loadcmd;

      if (need_file_transfer_)
      {
        if(!(file_transfer_->put_file(
          file_transfer_->local_file(input_field_matfile_[p]),
          file_transfer_->remote_file(input_field_matfile_[p]))))
        {
          module_->error("InterfaceWithMatlab: Could not transfer file");
          std::string err = "Error :" + file_transfer_->geterror();
          error(err);
          return(false);
        }
      }
      input_field_array_list_old_[p] = input_field_array_list_[p];
      input_field_name_list_old_[p] = input_field_name_list_[p];
      input_field_generation_old_[p] = handle->generation;
    }
#endif
#ifdef SCIRUN4_CODE_TO_BE_ENABLED_LATER
    for (int p = 0; p < NUM_NRRD_PORTS; p++)
    {
      NrrdDataHandle	handle;
      if(!(get_input_handle(port,handle,false))) { port++; continue; }
      port++;

      // if there is no data
      if (handle.get_rep() == 0)
      {
        // we do not need the old file any more so delete it
        input_nrrd_matfile_[p] = "";
        continue;
      }
      // if the data as the same before
      // do nothing
      if ((input_nrrd_name_list_[p]==input_nrrd_name_list_old_[p])&&
        (input_nrrd_type_list_[p]==input_nrrd_type_list_old_[p])&&
        (input_nrrd_array_list_[p]==input_nrrd_array_list_old_[p])&&
        (handle->generation == input_nrrd_generation_old_[p]))
      {
        // this one was not created again
        // hence we do not need to translate it again
        // with big datasets this should improve performance
        loadcmd = "load " + file_transfer_->remote_file(input_nrrd_matfile_[p]) + ";\n";
        m_file << loadcmd;

        continue;
      }

      // Create a new filename for the input matrix
      std::ostringstream oss;
      oss << "input_nrrd" << p << ".mat";
      input_nrrd_matfile_[p] = oss.str();

      matlabfile mf;
      matlabarray ma;

      mf.open(file_transfer_->local_file(input_nrrd_matfile_[p]),"w");
      mf.setheadertext("InterfaceWithMatlab V5 compatible file generated by SCIRun [module InterfaceWithMatlab version 1.3]");

      matlabconverter translate(dynamic_cast<SCIRun::ProgressReporter *>(this));
      translate.converttostructmatrix();
      if (input_nrrd_array_list_[p] == "numeric array") translate.converttonumericmatrix();
      translate.setdatatype(convertdataformat(input_nrrd_type_list_[p]));
      translate.sciNrrdDataTOmlArray(handle,ma);
      mf.putmatlabarray(ma,input_nrrd_name_list_[p]);
      mf.close();

      loadcmd = "load " + file_transfer_->remote_file(input_nrrd_matfile_[p]) + ";\n";
      m_file << loadcmd;

      if (need_file_transfer_)
      {
        if(!(file_transfer_->put_file(
          file_transfer_->local_file(input_nrrd_matfile_[p]),
          file_transfer_->remote_file(input_nrrd_matfile_[p]))))
        {
          error("InterfaceWithMatlab: Could not transfer file");
          std::string err = "Error :" + file_transfer_->geterror();
          error(err);
          return(false);
        }
      }
      input_nrrd_type_list_old_[p] = input_nrrd_type_list_[p];
      input_nrrd_array_list_old_[p] = input_nrrd_array_list_[p];
      input_nrrd_name_list_old_[p] = input_nrrd_name_list_[p];
      input_nrrd_generation_old_[p] = handle->generation;
    }
#endif
#if 0
    for (int p = 0; p < NUM_STRING_PORTS; p++)
    {
      StringHandle	handle;
      if(!(get_input_handle(port,handle,false))) { port++; continue; }
      port++;

      // if there is no data
      if (!handle)
      {
        // we do not need the old file any more so delete it
        input_string_matfile_[p].clear();
        continue;
      }
      // if the data as the same before
      // do nothing
      if ((input_string_name_list_[p]==input_string_name_list_old_[p])&&
        (handle->generation == input_string_generation_old_[p]))
      {
        // this one was not created again
        // hence we do not need to translate it again
        // with big datasets this should improve performance
        loadcmd = "load " + file_transfer_->remote_file(input_string_matfile_[p]) + ";\n";
        m_file << loadcmd;

        continue;
      }

      // Create a new filename for the input matrix
      std::ostringstream oss;
      oss << "input_string" << p << ".mat";
      input_string_matfile_[p] = oss.str();

      matlabfile mf;
      matlabarray ma;

      mf.open(file_transfer_->local_file(input_string_matfile_[p]),"w");
      mf.setheadertext("InterfaceWithMatlab V5 compatible file generated by SCIRun [module InterfaceWithMatlab version 1.3]");

      matlabconverter translate(module_->getLogger());
      translate.sciStringTOmlArray(handle,ma);
      mf.putmatlabarray(ma,input_string_name_list_[p]);
      mf.close();

      loadcmd = "load " + file_transfer_->remote_file(input_string_matfile_[p]) + ";\n";
      m_file << loadcmd;

      if (need_file_transfer_)
      {
        if(!(file_transfer_->put_file(
          file_transfer_->local_file(input_string_matfile_[p]),
          file_transfer_->remote_file(input_string_matfile_[p]))))
        {
          module_->error("InterfaceWithMatlab: Could not transfer file");
          std::string err = "Error :" + file_transfer_->geterror();
          error(err);
          return(false);
        }

      }
      input_string_name_list_old_[p] = input_string_name_list_[p];
      input_string_generation_old_[p] = handle->generation;
    }
#endif
  }
  catch (matlabfile::could_not_open_file&)
  {   // Could not open the temporary file
    module_->error("InterfaceWithMatlab: Could not open temporary matlab file");
    return(false);
  }
  catch (matlabfile::io_error&)
  {   // IO error from ferror
    module_->error("InterfaceWithMatlab: IO error");
    return(false);
  }
  catch (matlabfile::matfileerror&)
  {   // All other errors are classified as internal
    // matfileerrror is the base class on which all
    // other exceptions are based.
    module_->error("InterfaceWithMatlab: Internal error in writer");
    return(false);
  }

  return(true);
}

bool InterfaceWithMatlabImpl::create_temp_directory()
{
  if (temp_directory_.empty())
  {
    return (tfmanager_.create_tempdir("matlab-engine.XXXXXX", temp_directory_));
  }
  return(true);
}


bool InterfaceWithMatlabImpl::delete_temp_directory()
{
  if (temp_directory_ != "") tfmanager_.delete_tempdir(temp_directory_);
  temp_directory_ = "";
  return(true);
}

std::string InterfaceWithMatlabImpl::totclstring(const std::string &instring)
{
  size_t strsize = instring.size();
  int specchar = 0;
  for (int p = 0; p < strsize; p++)
    if ((instring[p] == '\n') || (instring[p] == '\t') || (instring[p] == '\b') || (instring[p] == '\r') || (instring[p] == '{') || (instring[p] == '}')
      || (instring[p] == '[') || (instring[p] == ']') || (instring[p] == '\\') || (instring[p] == '$') || (instring[p] == '"')) specchar++;

  std::string newstring;
  newstring.resize(strsize + specchar);
  int q = 0;
  for (int p = 0; p < strsize; p++)
  {
    if (instring[p] == '\n') { newstring[q++] = '\\'; newstring[q++] = 'n'; continue; }
    if (instring[p] == '\t') { newstring[q++] = '\\'; newstring[q++] = 't'; continue; }
    if (instring[p] == '\b') { newstring[q++] = '\\'; newstring[q++] = 'b'; continue; }
    if (instring[p] == '\r') { newstring[q++] = '\\'; newstring[q++] = 'r'; continue; }
    if (instring[p] == '{')  { newstring[q++] = '\\'; newstring[q++] = '{'; continue; }
    if (instring[p] == '}')  { newstring[q++] = '\\'; newstring[q++] = '}'; continue; }
    if (instring[p] == '[')  { newstring[q++] = '\\'; newstring[q++] = '['; continue; }
    if (instring[p] == ']')  { newstring[q++] = '\\'; newstring[q++] = ']'; continue; }
    if (instring[p] == '\\') { newstring[q++] = '\\'; newstring[q++] = '\\'; continue; }
    if (instring[p] == '$')  { newstring[q++] = '\\'; newstring[q++] = '$'; continue; }
    if (instring[p] == '"')  { newstring[q++] = '\\'; newstring[q++] = '"'; continue; }
    newstring[q++] = instring[p];
  }

  return(newstring);
}

#if 0
void InterfaceWithMatlab::tcl_command(GuiArgs& args, void* userdata)
{
  if (args.count() > 1)
  {
    if (args[1] == "keystroke")
    {
      std::string str = args[2];
      if (str.size() == 1)
      {
        if (str[0] == '\r') str[0] = '\n';

        if (str[0] == '\b')
        {
          inputstring_ = inputstring_.substr(0,(inputstring_.size()-1));
        }
        else
        {
          inputstring_ += str;
        }

        if (str[0] == '\n')
        {
          if(!(send_input(inputstring_)))
          {
            error("InterfaceWithMatlab: Could not close matlab engine");
            return;
          }
          inputstring_ = "";
        }
      }
      else
      {
        std::string key = args[3];
        if (key == "Enter")
        {
          str = "\n";
          inputstring_ += str;
        }
        else if (key == "BackSpace")
        {
          inputstring_ = inputstring_.substr(0,(inputstring_.size()-1));
        }
        else if (key == "Tab") str = "\t";
        else if (key == "Return") str ="\r";

        if (str.size() == 1)
        {
          if (str[0] == '\n')
          {
            if(!(send_input(inputstring_)))
            {
              error("InterfaceWithMatlab: Could not close matlab engine");
              return;
            }
            inputstring_ = "";
          }
        }

      }
      return;
    }
    if (args[1] == "disconnect")
    {
      get_ctx()->reset();
      if(!(close_matlab_engine()))
      {
        error("InterfaceWithMatlab: Could not close matlab engine");
        return;
      }
      update_status("InterfaceWithMatlab engine not running\n");
      return;
    }


    if (args[1] == "connect")
    {

      if(!(close_matlab_engine()))
      {
        error("InterfaceWithMatlab: Could not close matlab engine");
        return;
      }

      update_status("InterfaceWithMatlab engine not running\n");

      // Synchronise input: translate TCL lists into C++ STL lists
      if (!(synchronise_input()))
      {
        error("InterfaceWithMatlab: Could not retreive GUI input");
        return;
      }

      // If we haven't created a temporary directory yet
      // we open one to store all temp files in
      if (!(create_temp_directory()))
      {
        error("InterfaceWithMatlab: Could not create temporary directory");
        return;
      }

      if (!(open_matlab_engine()))
      {
        error("InterfaceWithMatlab: Could not open matlab engine");
        return;
      }
      return;
    }

    if (args[1] == "configfile")
    {
      ServiceDBHandle servicedb = new ServiceDB;
      // load all services and find all makers
      servicedb->loadpackages();

      ServiceInfo *si = servicedb->getserviceinfo("matlabengine");
      configfile_.set(si->rcfile);
      reset_vars();
      return;
    }
  }

  Module::tcl_command(args, userdata);
}
#endif

ALGORITHM_PARAMETER_DEF(Matlab, MatlabCode);
ALGORITHM_PARAMETER_DEF(Matlab, MatlabPath);
ALGORITHM_PARAMETER_DEF(Matlab, InputMatrixNames);
ALGORITHM_PARAMETER_DEF(Matlab, InputMatrixTypes);
ALGORITHM_PARAMETER_DEF(Matlab, InputMatrixArrays);
ALGORITHM_PARAMETER_DEF(Matlab, OutputMatrixNames);
