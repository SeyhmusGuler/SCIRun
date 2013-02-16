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
 
#include <gtest/gtest.h>

#include <fstream>
#include <Core/Algorithms/Base/AlgorithmPreconditions.h>
#include <Core/Algorithms/Math/ReportMatrixInfo.h>
#include <Core/Datatypes/DenseMatrix.h>
#include <Core/Datatypes/MatrixComparison.h>
#include <Core/Datatypes/MatrixIO.h>

using namespace SCIRun::Core::Datatypes;
using namespace SCIRun::Core::Algorithms::Math;
using namespace SCIRun::Core::Algorithms;

namespace
{
  DenseMatrixHandle matrix1Dense()
  {
    DenseMatrixHandle m(new DenseMatrix(3, 4));
    for (int i = 0; i < m->rows(); ++ i)
      for (int j = 0; j < m->cols(); ++ j)
        (*m)(i, j) = 3.0 * i + j - 5;
    return m;
  }
  SparseRowMatrixHandle matrix1Sparse() 
  {
    SparseRowMatrixHandle m(new SparseRowMatrix(5,5));
    m->insert(0,0) = 1;
    m->insert(1,2) = -1;
    m->insert(4,4) = 2;
    //TODO: remove this when NonZeroIterator is ready
    m->makeCompressed();
    return m;
  }
}

TEST(ReportMatrixInfoAlgorithmTests, ReportsMatrixType)
{
  ReportMatrixInfoAlgorithm algo;

  MatrixHandle m(matrix1Dense());
  ReportMatrixInfoAlgorithm::Outputs result = algo.run(m);
  EXPECT_EQ("DenseMatrix", result.get<0>());
  m = matrix1Sparse();
  result = algo.run(m);
  EXPECT_EQ("SparseRowMatrix", result.get<0>());
  m.reset(new DenseColumnMatrix);
  result = algo.run(m);
  EXPECT_EQ("DenseColumnMatrix", result.get<0>());
}

TEST(ReportMatrixInfoAlgorithmTests, ReportsRowAndColumnCount)
{
  ReportMatrixInfoAlgorithm algo;

  MatrixHandle m(matrix1Dense());
  ReportMatrixInfoAlgorithm::Outputs result = algo.run(m);
  EXPECT_EQ(3, result.get<1>());
  EXPECT_EQ(4, result.get<2>());
}

TEST(ReportMatrixInfoAlgorithmTests, ReportsNumberOfElements)
{
  ReportMatrixInfoAlgorithm algo;

  MatrixHandle m(matrix1Dense());
  ReportMatrixInfoAlgorithm::Outputs result = algo.run(m);
  
  EXPECT_EQ(12, result.get<3>());
}

TEST(ReportMatrixInfoAlgorithmTests, ReportsNumberOfNonzeroElementsForSparse)
{
  ReportMatrixInfoAlgorithm algo;

  MatrixHandle m(matrix1Sparse());
  ReportMatrixInfoAlgorithm::Outputs result = algo.run(m);

  EXPECT_EQ(3, result.get<3>());
}

TEST(ReportMatrixInfoAlgorithmTests, ReportsMinimumAndMaximum)
{
  ReportMatrixInfoAlgorithm algo;

  MatrixHandle m(matrix1Dense());
  ReportMatrixInfoAlgorithm::Outputs result = algo.run(m);
  EXPECT_EQ(-5, result.get<4>());
  EXPECT_EQ(4, result.get<5>());
  m = matrix1Sparse();
  result = algo.run(m);
  EXPECT_EQ(-1, result.get<4>());
  EXPECT_EQ(2, result.get<5>());
}

TEST(ReportMatrixInfoAlgorithmTests, NullInputThrows)
{
  ReportMatrixInfoAlgorithm algo;

  EXPECT_THROW(algo.run(DenseMatrixHandle()), AlgorithmInputException);
}

TEST(ReportMatrixInfoAlgorithmTests, EmptyInputDoesNotThrow)
{
  ReportMatrixInfoAlgorithm algo;
  DenseMatrixHandle empty(new DenseMatrix);

  auto result = algo.run(empty);
  EXPECT_EQ(0, result.get<1>());
  EXPECT_EQ(0, result.get<2>());
  EXPECT_EQ(0, result.get<3>());
  EXPECT_EQ(0, result.get<4>());
  EXPECT_EQ(0, result.get<5>());
}

TEST(BigVectorFieldFile, DISABLED_MakeIt)
{
  const size_t dim = 10;
  const size_t cols = 3 * dim * dim * dim;
  DenseMatrix m(6, cols);
  size_t col = 0;
  for (int d = 0; d < 3; ++d)
  {
    for (int i = 0; i < dim; ++i)
    {
      for (int j = 0; j < dim; ++j)
      {
        for (int k = 0; k < dim; ++k)
        {
          m(0, col) = (col % 3) == 0;
          m(1, col) = (col % 3) == 1;
          m(2, col) = (col % 3) == 2;
          m(3, col) = i;
          m(4, col) = j;
          m(5, col) = k;
          col++;
        }
      }
    }
  }
  std::ofstream file("E:\\git\\SCIRunGUIPrototype\\src\\Samples\\danBigMatrix.txt");
  file << m;
}

TEST(BigVectorFieldFile, DISABLED_ReadIt)
{
  std::ifstream file("E:\\git\\SCIRunGUIPrototype\\src\\Samples\\danBigMatrix.txt");
  DenseMatrix m;
  file >> m;
  EXPECT_EQ(6, m.rows());
  EXPECT_EQ(3000, m.cols());
}