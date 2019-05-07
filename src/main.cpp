#include <iostream>
#include <mpi.h>

int main(int argc, char** argv)
{
	MPI_Init(&argc, &argv);
	std::cout << "Hey";
	MPI_Finalize();
}