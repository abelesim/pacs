
// @HEADER
// ***********************************************************************
// 
//                      Didasko Tutorial Package
//                 Copyright (2005) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
//
// Questions about Didasko? Contact Marzio Sala (marzio.sala _AT_ gmail.com)
// 
// ***********************************************************************
// @HEADER

// Basic definition of communicator.
// This code should be run with at least two processes

#include "Didasko_ConfigDefs.h"
#if defined(HAVE_DIDASKO_EPETRA) && defined(HAVE_DIDASKO_TRIUTILS)

#include "Epetra_ConfigDefs.h"
#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Vector.h"
#include "Epetra_RowMatrix.h"
#include "Trilinos_Util.h"

class MSRMatrix : public Epetra_Operator 
{

public:
  
  // constructor
  MSRMatrix(Epetra_Map Map, int * bindx, double * val) :
    bindx_(bindx), 
    val_(val),
    Map_(Map)
  {
  }

  // destructor, nothing to do
  ~MSRMatrix() 
  {}

  // Apply the RowMatrix to a MultiVector
  int Apply(const Epetra_MultiVector & X, Epetra_MultiVector & Y ) const 
  {

    int Nrows = bindx_[0]-1;
    
    for( int i=0 ; i<Nrows ; i++ ) {
      // diagonal element
      for( int vec=0 ; vec<X.NumVectors() ; ++vec ) {
	Y[vec][i] = val_[i]*X[vec][i];
      }
      // off-diagonal elements
      for( int j=bindx_[i] ; j<bindx_[i+1] ; j++ ) {
	for( int vec=0 ; vec<X.NumVectors() ; ++vec ) {
	  Y[vec][bindx_[j]] += val_[j]*X[vec][bindx_[j]];
	}
      }
    }

    return 0;
    
  } /* Apply */

  // other function, required by Epetra_RowMatrix. Here are almost all
  // void, you may decide to complete the example...
  int SetUseTranspose( bool UseTranspose)
  {
    return(-1); // not implemented
  }

  int ApplyInverse( const Epetra_MultiVector & X,
                    Epetra_MultiVector & Y ) const
  {
    return(-1); // not implemented
  }

  double NormInf() const
  {
    return -1;
  }

  const char* Label () const
  {
    return "TriDiagonalOperator";
  }

  bool UseTranspose() const
  {
    return false;
  }

  bool HasNormInf () const
  {
    return true;
  }


  const Epetra_Comm & Comm() const
  {
    return( Map_.Comm() );
  }

  const Epetra_Map & OperatorDomainMap() const
  {
    return( Map_ );
  }

  const Epetra_Map & OperatorRangeMap() const
  {
    return( Map_ );
  }

private:

  int * bindx_;    /* MSR vector for nonzero indices */
  double * val_;   /* MSR vector for nonzero values */
  Epetra_Map Map_;
  
}; /* MSRMatrix class */
  
// =========== //
// main driver //
// ----------- //
  
int main(int argc, char *argv[])
{

#ifdef HAVE_MPI
  MPI_Init(&argc, &argv);
  // define an Epetra communicator
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  // check number of processes
  if (Comm.NumProc() != 1) {
    if (Comm.MyPID() == 0)
      cerr << "*ERR* can be used only with one process" << endl;
#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    exit(EXIT_SUCCESS);
  }
    
  // process 0 will read an HB matrix, and store it
  // in the MSR format given by the arrays bindx and val
  int N_global;
  int N_nonzeros;
  double * val = NULL;
  int * bindx = NULL;
  double * x = NULL, * b = NULL, * xexact = NULL;
  
  FILE* fp = fopen("../HBMatrices/fidap005.rua", "r");
  if (fp == 0)
  {
    cerr << "Matrix file not available" << endl;
#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    exit(EXIT_SUCCESS);
  }
  fclose(fp);
  
  Trilinos_Util_read_hb("../HBMatrices/fidap005.rua", 0,
			&N_global, &N_nonzeros, 
			&val, &bindx,
			&x, &b, &xexact);

  // assign all the elements to process 0
  // (this code can run ONLY with one process, extensions to more
  // processes will require functions to handle update of ghost nodes)
  Epetra_Map Map(N_global,0,Comm);
  
  MSRMatrix A(Map,bindx,val);
  
  // define two vectors
  Epetra_Vector xxx(Map);
  Epetra_Vector yyy(Map);

  xxx.Random();

  A.Apply(xxx,yyy);

  cout << yyy;
  
  double norm2;
  yyy.Norm2(&norm2);

  cout << "Norm="<<norm2 << endl;

  // free memory allocated by Trilinos_Util_read_hb
  if (val != NULL) free((void*)val);
  if (bindx != NULL) free((void*)bindx);
  if (x != NULL) free((void*)x);
  if (b != NULL) free((void*)b);
  if (xexact != NULL) free((void*)xexact);;

#ifdef HAVE_MPI
  MPI_Finalize();
#endif

  return(EXIT_SUCCESS);
  
} /* main */

#else

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  puts("Please configure Didasko with:\n"
       "--enable-epetra\n"
       "--enable-triutils");

  return(0);
}
#endif
