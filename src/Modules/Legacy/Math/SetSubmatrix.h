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

#ifndef MODULES_LEGACY_MATH_SETSUBMATRIX_H_
#define MODULES_LEGACY_MATH_SETSUBMATRIX_H_ 1

#include <Dataflow/Network/Module.h>
#include <Modules/Legacy/Math/share.h>

namespace SCIRun {
	namespace Core
	{
		namespace Algorithms
		{
			namespace Math
			{
				ALGORITHM_PARAMETER_DECL(StartColumn);
				ALGORITHM_PARAMETER_DECL(StartRow);
				ALGORITHM_PARAMETER_DECL(MatrixDims);
				ALGORITHM_PARAMETER_DECL(SubmatrixDims);
			}
		}
	}

	namespace Modules {
		namespace Math {

		class SCISHARE SetSubmatrix : public Dataflow::Networks::Module,
			public Has3InputPorts<MatrixPortTag, MatrixPortTag, MatrixPortTag>,
			public Has1OutputPort<MatrixPortTag>
			{
				public:
					SetSubmatrix();
					virtual void setStateDefaults() override;
					virtual void execute() override;

					INPUT_PORT(0, InputMatrix, Matrix);
					INPUT_PORT(1, Input_Submatrix, Matrix);
					INPUT_PORT(2, Optional_Start_Bounds, Matrix);
					OUTPUT_PORT(0, OutputMatrix, Matrix);

          static const Dataflow::Networks::ModuleLookupInfo staticInfo_;

			};

}}};

#endif
