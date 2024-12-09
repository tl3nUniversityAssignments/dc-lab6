#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <mpi.h>
#include <algorithm>

static int ProcNum = 0; // Number of available processes
static int ProcRank = -1; // Rank of current process

// Function for distribution of the grid rows among the processes
void DataDistribution(double* pMatrix, double* pProcRows, int Size, int RowNum) {
    int *pSendNum; // Number of elements sent to the process
    int *pSendInd; // Index of the first data element sent to the process
    int RestRows=Size;

    // Alloc memory for temporary objects
    pSendInd = new int [ProcNum];
    pSendNum = new int [ProcNum];

    // Define the disposition of the matrix rows for current process
    RowNum = (Size-2)/ProcNum+2;
    pSendNum[0] = RowNum*Size;
    pSendInd[0] = 0;

    for (int i=1; i<ProcNum; i++) {
        RestRows = RestRows - RowNum + 1;
        RowNum = (RestRows-2)/(ProcNum-i)+2;
        pSendNum[i] = (RowNum)*Size;
        pSendInd[i] = pSendInd[i-1]+pSendNum[i-1]-Size;
    }

    // Scatter the rows
    MPI_Scatterv(pMatrix , pSendNum, pSendInd, MPI_DOUBLE, pProcRows,
    pSendNum[ProcRank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

    delete []pSendInd;
    delete []pSendNum;
}

// Function for computational process termination
void ProcessTermination (double* pMatrix, double* pProcRows) {
    if (ProcRank == 0)
        delete [] pMatrix;
    delete [] pProcRows;
}

// Function for formatted matrix output
void PrintMatrix(double *pMatrix, int RowCount, int ColCount){
    int i,j; // Loop variables
    for(int i=0; i < RowCount; i++) {
        for(j=0; j < ColCount; j++)
            printf("%7.4f ", pMatrix[i*ColCount+j]);
        printf("\n");
    }
}

// Function for the execution of the Gauss-Seidel method iteration
double IterationCalculation(double* pProcRows, int Size, int RowNum) {
    int i, j; // Loop variables
    double dm, dmax,temp;
    dmax = 0;

    for (i = 1; i < RowNum-1; i++)
        for(j = 1; j < Size-1; j++) {
            temp = pProcRows[Size * i + j];
            pProcRows[Size * i + j] = 0.25 * (pProcRows[Size * i + j + 1] +
            pProcRows[Size * i + j - 1] +
            pProcRows[Size * (i + 1) + j] +
            pProcRows[Size * (i - 1) + j]);
            dm = fabs(pProcRows[Size * i + j] - temp);
            if (dmax < dm) dmax = dm;
        }

    return dmax;
}

// Function for testing the data distribution
void TestDistribution(double* pMatrix, double* pProcRows, int Size,
int RowNum) {
    if (ProcRank == 0) {
        printf("Initial Matrix: \n");
        PrintMatrix(pMatrix, Size, Size);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i=0; i<ProcNum; i++) {
        if (ProcRank == i) {
            printf("\nProcRank = %d \n", ProcRank);
            // fprintf(" Matrix Stripe:\n");
            PrintMatrix(pProcRows, RowNum, Size);
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }
}

// Function for simple setting the grid node values
void DummyDataInitialization (double* pMatrix, int Size) {
    int i, j; // Loop variables
    double h = 1.0 / (Size - 1);

    // Setting the grid node values
    for (i=0; i<Size; i++) {
        for (j=0; j<Size; j++)
            if ((i==0) || (i== Size-1) || (j==0) || (j==Size-1))
                pMatrix[i*Size+j] = 100;
            else
                pMatrix[i*Size+j] = 0;
    }
}

// Function for memory allocation and initialization of grid nodes
void ProcessInitialization (double* &pMatrix, double* &pProcRows,int &Size,
int &RowNum, double &Eps) {
    int RestRows; // Number of rows, that haven’t been distributed yet

    // Setting the grid size
    if (ProcRank == 0) {
        do {
            printf("\nEnter the grid size: ");
            scanf("%d", &Size);

            if (Size <= 2) {
                printf("\n Size of grid must be greater than 2! \n");
            }

            if (Size < ProcNum) {
                printf("Size of grid must be greater than"
                "the number of processes! \n ");
            }
        } while ( (Size <= 2) || (Size < ProcNum));

        // Setting the required accuracy
        do {
            printf("\nEnter the required accuracy: ");
            scanf("%lf", &Eps);
            printf("\nChosen accuracy = %lf", Eps);
            if (Eps <= 0)
                printf("\nAccuracy must be greater than 0!\n");
        } while (Eps <= 0);
    }

    MPI_Bcast(&Size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&Eps, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Define the number of matrix rows stored on each process
    RestRows = Size;
    for (int i=0; i<ProcRank; i++)
        RestRows = RestRows-RestRows/(ProcNum-i);
    RowNum = (RestRows-2)/(ProcNum-ProcRank)+2;

    // Memory allocation
    pProcRows = new double [RowNum*Size];

    // Define the values of initial objects’ elements
    if (ProcRank == 0) {
        // Initial matrix exists only on the pivot process
        pMatrix = new double [Size*Size];

        // Values of elements are defined only on the pivot process
        DummyDataInitialization(pMatrix, Size);
    }
}

// Function for memory allocation and initialization of grid nodes
void ProcessTestInitialization (double* &pMatrix, double* &pProcRows,int &Size,
int &RowNum, double &Eps) {
    int RestRows; // Number of rows, that haven’t been distributed yet

    // Setting the grid size
    if (ProcRank == 0) {
        if (Size <= 2) {
            printf("\n Size of grid must be greater than 2! \n");
        }

        if (Size < ProcNum) {
            printf("Size of grid must be greater than"
            "the number of processes! \n ");
        }
        // Setting the required accuracy
        if (Eps <= 0)
            printf("\nAccuracy must be greater than 0!\n");
    }

    MPI_Bcast(&Size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&Eps, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Define the number of matrix rows stored on each process
    RestRows = Size;
    for (int i=0; i<ProcRank; i++)
        RestRows = RestRows-RestRows/(ProcNum-i);
    RowNum = (RestRows-2)/(ProcNum-ProcRank)+2;

    // Memory allocation
    pProcRows = new double [RowNum*Size];

    // Define the values of initial objects’ elements
    if (ProcRank == 0) {
        // Initial matrix exists only on the pivot process
        pMatrix = new double [Size*Size];

        // Values of elements are defined only on the pivot process
        DummyDataInitialization(pMatrix, Size);
    }
}

// Function for exchanging the boundary rows of the process stripes
void ExchangeData(double* pProcRows, int Size, int RowNum) {
    MPI_Status status;

    int NextProcNum = (ProcRank == ProcNum-1)? MPI_PROC_NULL : ProcRank + 1;
    int PrevProcNum = (ProcRank == 0)? MPI_PROC_NULL : ProcRank - 1;

    // Send to NextProcNum and receive from PrevProcNum
    MPI_Sendrecv(pProcRows + Size * (RowNum - 2),Size, MPI_DOUBLE,
    NextProcNum, 4, pProcRows, Size, MPI_DOUBLE, PrevProcNum, 4,
    MPI_COMM_WORLD, &status);

    // Send to PrevProcNum and receive from NextProcNum
    MPI_Sendrecv(pProcRows + Size, Size, MPI_DOUBLE, PrevProcNum, 5,
    pProcRows + (RowNum - 1) * Size, Size,MPI_DOUBLE, NextProcNum, 5,
    MPI_COMM_WORLD, &status);
}

// Function for the parallel Gauss - Seidel method
void ParallelResultCalculation (double *pProcRows, int Size, int RowNum,
double Eps, int &Iterations) {
    double ProcDelta,Delta;
    Iterations=0;

    do {
        Iterations++;

        // Exchanging the boundary rows of the process stripe
        ExchangeData(pProcRows, Size,RowNum);

        // The Gauss-Seidel method iteration
        ProcDelta = IterationCalculation(pProcRows, Size, RowNum);

        // Calculating the maximum value of the deviation
        MPI_Allreduce(&ProcDelta, &Delta, 1,MPI_DOUBLE, MPI_MAX,
        MPI_COMM_WORLD);
    } while ( Delta > Eps);
}

// Function for gathering the result vector
void ResultCollection(double *pMatrix, double* pProcRows, int Size,
int RowNum) {
    int *pReceiveNum; // Number of elements, that current process sends
    int *pReceiveInd; // Index of the first element of the received block
    int RestRows = Size;
    int i; // Loop variable

    // Alloc memory for temporary objects
    pReceiveNum = new int [ProcNum];
    pReceiveInd = new int [ProcNum];

    // Define the disposition of the result vector block of current processor
    pReceiveInd[0] = 0;
    RowNum = (Size-2)/ProcNum+2;
    pReceiveNum[0] = RowNum*Size;
    
    for ( i=1; i < ProcNum; i++){
        RestRows = RestRows - RowNum + 1;
        RowNum = (RestRows-2)/(ProcNum-i)+2;
        pReceiveNum[i] = RowNum*Size;
        pReceiveInd[i] = pReceiveInd[i-1]+pReceiveNum[i-1]-Size;
    }

    // Gather the whole result vector on every processor
    MPI_Gatherv(pProcRows, pReceiveNum[ProcRank], MPI_DOUBLE, pMatrix,
    pReceiveNum, pReceiveInd, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // Free the memory
    delete [] pReceiveNum;
    delete [] pReceiveInd;
}

// Function for the serial Gauss – Seidel method
void SerialResultCalculation(double *pMatrixCopy, int Size, double Eps,
int &Iter){
    int i, j; // Loop variables
    double dm, dmax,temp;
    Iter = 0;

    do {
        dmax = 0;
        for (i = 1; i < Size - 1; i++)
            for(j = 1; j < Size - 1; j++) {
                temp = pMatrixCopy[Size * i + j];
                pMatrixCopy[Size * i + j] = 0.25 * (pMatrixCopy[Size * i + j + 1] +
                pMatrixCopy[Size * i + j - 1] +
                pMatrixCopy[Size * (i + 1) + j] +
                pMatrixCopy[Size * (i - 1) + j]);
                dm = fabs(pMatrixCopy[Size * i + j] - temp);
                if (dmax < dm) dmax = dm;
            }
        Iter++;
    }
    while (dmax > Eps);
}

// Function to copy the initial data
void CopyData(double *pMatrix, int Size, double *pSerialMatrix) {
    std::copy(pMatrix, pMatrix + Size, pSerialMatrix);
}

// Function for testing the computation result
void TestResult(double* pMatrix, double* pSerialMatrix, int Size,
double Eps) {
    int equal = 0; // =1, if the matrices are not equal
    int Iter;

    if (ProcRank == 0) {
        SerialResultCalculation(pSerialMatrix, Size, Eps, Iter);
        for (int i=0; i<Size*Size; i++) {
            if (fabs(pSerialMatrix[i]-pMatrix[i]) >= Eps)
                equal = 1;
            break;
        }

        if (equal == 1)
            printf("The results of serial and parallel algorithms"
            "are NOT identical. Check your code.");
        else
            printf("The results of serial and parallel algorithms"
            "are identical.");
    }
}

// Function for setting the grid node values by a random generator
void RandowmDataInitialization (double* pMatrix, int Size) {
    int i, j; // Loop variables

    srand(unsigned(clock()));

    // Setting the grid node values
    for (i=0; i<Size; i++) {
        for (j=0; j<Size; j++)
            if ((i==0) || (i== Size-1) || (j==0) || (j==Size-1))
                pMatrix[i*Size+j] = 100;
            else
                pMatrix[i*Size+j] = rand()/double(1000);
    }
}

void test(int argc, char* argv[]) {
    double* pMatrix; // Matrix of the grid nodes
    double* pProcRows; // Stripe of the matrix on current process
    double* pSerialMatrix; // Result of the serial method
    int Size; // Matrix size
    int sizes[] = {10, 100, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, 10000};
    int RowNum; // Number of rows in matrix stripe
    double Eps = 10; // Required accuracy
    int Iterations; // Iteration number
    double Start, Finish, Duration; // For time measurement

    setvbuf(stdout, 0, _IONBF, 0);
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    // Only root process handles output
    if (ProcRank == 0) {
        printf("Parallel Gauss-Seidel Cyclic Performance Test\n");
    }

    // TEST 1-12
    for (int i = 0; i < 12; i++) {
        Size = sizes[i];

        // Process initialization
        ProcessTestInitialization(pMatrix, pProcRows, Size, RowNum, Eps);

        // Creating the copy of the initial data
        if (ProcRank == 0) {
            pSerialMatrix = new double[Size*Size];
            CopyData(pMatrix, Size, pSerialMatrix);
        }

        // Start time measurement
        Start = MPI_Wtime();

        // Data distribution among the processes
        DataDistribution(pMatrix, pProcRows, Size, RowNum);

        // Parallel Gauss-Seidel method
        ParallelResultCalculation(pProcRows, Size, RowNum, Eps, Iterations);

        // Gathering the calculation results
        ResultCollection(pMatrix, pProcRows, Size, RowNum);

        // End time measurement and output results on the root process
        Finish = MPI_Wtime();
        Duration = Finish - Start;

        if (ProcRank == 0) {
            printf("\nTime: %f, Size: %d, Iterations: %d", Duration, Size, Iterations);
        }

        // Process termination
        if (ProcRank == 0) delete []pSerialMatrix;
        ProcessTermination(pMatrix, pProcRows);
    }

}

int main(int argc, char* argv[]) {
    // double* pMatrix; // Matrix of the grid nodes
    // double* pProcRows; // Stripe of the matrix on current process
    // double* pSerialMatrix; // Result of the serial method
    // int Size; // Matrix size
    // int RowNum; // Number of rows in matrix stripe
    // double Eps; // Required accuracy
    // int Iterations; // Iteration number
    // double currDelta, delta;

    // setvbuf(stdout, 0, _IONBF, 0);
    // MPI_Init(&argc, &argv);
    // MPI_Comm_size(MPI_COMM_WORLD, &ProcNum);
    // MPI_Comm_rank(MPI_COMM_WORLD, &ProcRank);

    // if(ProcRank == 0) {
    // printf("Parallel Gauss - Seidel algorithm \n");
    // fflush(stdout);
    // }

    // // Process initialization
    // ProcessInitialization (pMatrix,pProcRows,Size,RowNum,Eps);

    // // Creating the copy of the initial data
    // if (ProcRank == 0) {
    // pSerialMatrix = new double[Size*Size];
    // CopyData(pMatrix, Size, pSerialMatrix);
    // }

    // // Data distribution among the processes
    // DataDistribution(pMatrix, pProcRows, Size,RowNum);

    // // Paralle Gauss-Seidel method
    // ParallelResultCalculation(pProcRows, Size,RowNum,Eps, Iterations);
    // //TestDistribution(pMatrix, pProcRows, Size,RowNum);

    // // Gathering the calculation results
    // ResultCollection(pProcRows, pMatrix, Size, RowNum);
    // TestDistribution(pMatrix, pProcRows, Size,RowNum);

    // // Printing the result
    // printf("\n Iter %d \n", Iterations);
    // printf("\nResult matrix: \n");
    // if (ProcRank==0) {
    // //TestResult(pMatrix,Size,pMatrixCopy,Eps);
    // PrintMatrix(pMatrix,Size,Size);
    // }

    // // Process termination
    // if (ProcRank == 0) delete []pSerialMatrix;
    // ProcessTermination(pMatrix, pProcRows);
    // MPI_Finalize();
    test(argc, argv);
    MPI_Finalize();
    return 0;
}
    