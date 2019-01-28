#include "utility/svd_utility.h"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
using namespace std;

// applies SVD and returns V transposed
// m columns and n rows
std::vector<double> SvdUtility::getSVD(vector<double> aData, int m, int n)
{
  /*
   * lda = ldu = length(column)
   * ldvt= length(row)
   * s = singular values
   * superb = array to store intermediate results
   * a = matrix which we want to decomposite as 1D-array
   * 
   * */



  double a[aData.size()];
  copy(aData.begin(), aData.end(), a);
  // Spalten    Zeilen
  // int m = 6, n = 5;

  /* double a[m*n] = {
            8.79,  6.11, -9.15,  9.57, -3.49,  9.84,
            9.93,  6.91, -7.93,  1.64,  4.02,  0.15,
            9.83,  5.04,  4.86,  8.83,  9.80, -8.99,
            5.45, -0.27,  4.85,  0.74, 10.00, -6.02,
            3.16,  7.98,  3.01,  5.80,  4.27, -5.31
        };
  **/
  int lda = m, ldu = m, ldvt = n;
  int matrix_order = LAPACK_COL_MAJOR;
  //int matrix_order = LAPACK_ROW_MAJOR;
  int minmn = std::min(m,n) - 1;
  double s[n], u[ldu*m], vt[ldvt*n], superb[minmn];

  int info = LAPACKE_dgesvd(matrix_order, 'a', 'a', m, n, a, lda, s, u, ldu, vt, ldvt, superb);

  std::cout << "info: " << info << endl;

  for(int i = 0; i < ldvt*n; i++)
    {
      cout << vt[i] << endl;
    }
  
  return std::vector<double>(vt, vt + sizeof vt / sizeof vt[0]);;
}

// takes input data as double array and writes Sigma, U and V transposed as output parameter double arrays
//m columns, n rows
void SvdUtility::getSVD(double a[], int m, int n, double u[], double s[], double vt[])
{
  //double a[aData.size()];
  //copy(aData.begin(), aData.end());

  std::cout << m << " columns, " << n << " rows" << endl;

  int lda = m, ldu = m, ldvt = n;
  int matrix_order = LAPACK_COL_MAJOR;
  int minmn = std::min(m,n) - 1;
  double superb[minmn];

  int info = LAPACKE_dgesvd(matrix_order, 'a', 'a', m, n, a, lda, s, u, ldu, vt, ldvt, superb);

  std::cout << "info: " << info << endl;
  /*
  std::cout << "Left singular vectors:" << endl;

  for(int i=0;i<m*n;++i)
  {
    std::cout << u[i] << endl;
  }
  */
  std::cout << "getSVD done" << endl;
}

// reads CSV cell by cell as vector
std::vector<double> SvdUtility::readCSV(string filename)
{
  ifstream data(filename);
  string line;
  vector<double> parsedCsv;
  int i, j = 0;
  while(getline(data, line))
    {
      stringstream lineStream(line);
      i++;
      string cell;
      j = 0;
      while(getline(lineStream,cell,','))
	{
	  parsedCsv.push_back(stof(cell));
	  j++;
	}
      // cout << i << endl;
      // cout << j << endl;
    }
  return parsedCsv;
}

// reads specified rows of CSV cell by cell as vector
std::vector<double> SvdUtility::readCSV(string filename, int rows)
{
  ifstream data(filename);
  string line;
  vector<double> parsedCsv;
  for(int i = 0; i < rows; i++)
    {
      getline(data, line);
      stringstream lineStream(line);
      string cell;
      while(getline(lineStream,cell,','))
	{
	  parsedCsv.push_back(stof(cell));
	}
    }
  return parsedCsv;
}

// writes vector cell by cell as CSV
void SvdUtility::writeCSV(string filename, std::vector<double> values, int m, int n)
{
  ofstream data;
  data.open(filename);
  for(int i = 0; i < m; i++)
    {
      for(int j = 0; j < n; j++)
	{
	  data << std::to_string(values[i*m + j]) << ",";
	}
      data << "\n";
    }
  data.close();
}

// returns number of rows of CSV
int SvdUtility::getCSVRowCount(string filename)
{
  ifstream data(filename);
  string line;
  int i = 0;
  while(getline(data, line))
    {
      stringstream lineStream(line);
      i++;
    }
  return i;
}

// returns number of columns of CSV
int SvdUtility::getCSVColumnCount(string filename)
{
  ifstream data(filename);
  string line;
  vector<double> parsedCsv;
  int j = 0;
  if(getline(data, line))
    {
      stringstream lineStream(line);
      string cell;
      while(getline(lineStream,cell,','))
	{
	  parsedCsv.push_back(stof(cell));
	  j++;
	}
    }
  return j;
}
