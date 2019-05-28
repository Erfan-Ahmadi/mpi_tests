#include <iostream>
#include <mpi.h>
#include <time.h>
#include <chrono>
#include <vector>

//#define MEM_STATIC
#define PRINT_VALIDATION

// Constants
auto constexpr num_runs = 50;
auto constexpr n = 512;
auto constexpr static_size = 64;

#ifdef MEM_STATIC
int initialize_array(my_type matrix[n])
#else
int initialize_array(int*& matrix)
#endif
{
	if (matrix == nullptr)
	{
		std::cout << "Could not allocate memory for matrix!";
		return 1;
	}

	for (auto i = 0; i < n * n; ++i)
		matrix[i] = rand() % 10000;

	return 0;
}

int main(int argc, char** argv)
{
	srand(time(NULL));

	int provided = -1;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

	if (provided != MPI_THREAD_MULTIPLE)
	{
		std::cout << "Couldn't run multi-threaded.";
		MPI_Finalize();
		return 0;
	}

	int rank, com_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &com_size);

	int local_sum = 0;

	double start = 0;

	double start_time = MPI_Wtime();

	if (rank == 0)
	{
		const size_t size = n * n;
		int* data = static_cast<int*>(malloc(size * sizeof(int)));

#ifdef MEM_STATIC
		my_type matrix[n];
#else
		int* matrix = nullptr;
		matrix = static_cast<int*>(malloc(sizeof(int) * size));
#endif
		initialize_array(matrix);

#if defined(PRINT_VALIDATION)
		int validation = 0;
		
		double ss = MPI_Wtime();
		for (size_t i = 0; i < size; ++i)
			validation += matrix[i];
		double ee = MPI_Wtime();
		std::cout << "sequential time: " << (ee - ss) * 1000 << "ms" << std::endl;
		std::cout << "result should be : " << validation << std::endl;
#endif
		start_time = MPI_Wtime();

		int div = size / com_size; // I'm sure it's divisible
		int send_size[1] = { div };

		for (size_t i = 1; i < com_size; ++i)
		{
			MPI_Request req;
			MPI_Send(send_size, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Isend(&matrix[div * i], div, MPI_INT, i, 0, MPI_COMM_WORLD, &req);
		}

		for (size_t i = 0; i < div; ++i)
			local_sum += matrix[i];
	}
	else
	{
		MPI_Status status;
		int recv_size[1];
		MPI_Recv(recv_size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

		int* matrix = static_cast<int*>(malloc(sizeof(int) * recv_size[0]));

		MPI_Recv(matrix, recv_size[0], MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		for (size_t i = 0; i < recv_size[0]; ++i)
		{
			local_sum += matrix[i];
		}
	}

	//std::cout << "local sum (" << rank << ") = " << local_sum << std::endl;

	int all_sum = 0;
	MPI_Reduce(&local_sum, &all_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

	if (rank == 0) 
	{
		double end_time = MPI_Wtime();

		std::cout << "The process took " << (end_time - start_time) * 1000 << " ms" << std::endl;
		std::cout << "all sum = " << all_sum << std::endl;
	}

	MPI_Finalize();
}


// Dynamic Memory Allocation 
// Intel(R) Core(TM) i5-8250U CPU
// 8 Threads

/*
Initialization - Init Duration =  27.935(ms)
---------------------------
Sequential										Average Duration =  0.5436(ms)
---------------------------
OPENMP Parallel For Reduction					Average Duration =  0.1382(ms)
---------------------------
OPENMP Parallel For Reduction Static (64)		Average Duration =  0.13492(ms)
---------------------------
OPENMP Parallel For Reduction Dynamic	        Average Duration =  0.23542(ms)
---------------------------
MPI Sum																3.03414 ms

 ***** MPI Slower Due To Running in Shared Memory System and Additional Memory Copying and allocating. ******
*/