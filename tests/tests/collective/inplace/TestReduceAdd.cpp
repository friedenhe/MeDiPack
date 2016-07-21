#include <toolDefines.h>

IN(10)
OUT(10)
POINTS(1) = {{{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0}}};
SEEDS(1) = {{{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}, {11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0}}};

void func(NUMBER* x, NUMBER* y) {
  int world_rank;
  TAMPI_Comm_rank(TAMPI_COMM_WORLD, &world_rank);
  int world_size;
  TAMPI_Comm_size(TAMPI_COMM_WORLD, &world_size);

  if(world_rank == 0) {
    for(int i = 0; i < 10; ++i) {
      y[i] = x[i];
    }
    medi::TAMPI_Reduce<MPI_NUMBER>(static_cast<NUMBER*>(TAMPI_IN_PLACE), y, 10, TOOL::OP_ADD, 0, MPI_COMM_WORLD);
  } else {
    medi::TAMPI_Reduce<MPI_NUMBER>(x, NULL, 10, TOOL::OP_ADD, 0, MPI_COMM_WORLD);
  }
}