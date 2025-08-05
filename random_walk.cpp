#include <iostream>
#include <cstdlib> // For atoi, rand, srand
#include <ctime>   // For time
#include <mpi.h>

void walker_process();
void controller_process();

int domain_size;
int max_steps;
int world_rank;
int world_size;

int main(int argc, char **argv)
{
    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes and the rank of this process
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    if (argc != 3)
    {
        if (world_rank == 0)
        {
            std::cerr << "Usage: mpirun -np <p> " << argv[0] << " <domain_size> <max_steps>" << std::endl;
        }
        MPI_Finalize();
        return 1;
    }

    domain_size = atoi(argv[1]);
    max_steps = atoi(argv[2]);

    if (world_rank == 0)
    {
        // Rank 0 is the controller
        controller_process();
    }
    else
    {
        // All other ranks are walkers
        walker_process();
    }

    // Finalize the MPI environment
    MPI_Finalize();
    return 0;
}
void walker_process() {
    srand(time(NULL) + world_rank);

    int position = 0;
    int steps_taken = 0;

    for (int i = 0; i < max_steps; ++i) {
        // Random step: -1 (left) or +1 (right)
        int step = (rand() % 2 == 0) ? -1 : 1;
        position += step;
        steps_taken++;

        if (position < -domain_size || position > domain_size) {
            // Walker has moved out of bounds
            std::cout << "Rank " << world_rank << ": Walker finished in " << steps_taken << " steps." << std::endl;

            // Send completion signal to controller (just sending number of steps)
            MPI_Send(&steps_taken, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            break;
        }

        // If max steps reached without going out of bounds
        if (i == max_steps - 1) {
            std::cout << "Rank " << world_rank << ": Walker finished in " << steps_taken << " steps (max steps reached)." << std::endl;
            MPI_Send(&steps_taken, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
}

void controller_process() {
    int num_walkers = world_size - 1;
    int completed_walkers = 0;

    for (int i = 0; i < num_walkers; ++i) {
        int steps_received;
        MPI_Status status;

        // Receive from any source
        MPI_Recv(&steps_received, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

        std::cout << "Controller: Received completion from Rank " << status.MPI_SOURCE
                  << " (steps taken: " << steps_received << ")" << std::endl;

        completed_walkers++;
    }

    std::cout << "Controller: All " << completed_walkers << " walkers have finished." << std::endl;
}
