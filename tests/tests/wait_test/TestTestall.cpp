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

  medi::TAMPI_Request request[10];
  for(int i = 0; i < 10; ++i) {
    if(world_rank == 0) {
      medi::TAMPI_Isend(&x[i], 1, mpiNumberType, 1, 42 + i, TAMPI_COMM_WORLD, &request[i]);
    } else {
      medi::TAMPI_Irecv(&y[i], 1, mpiNumberType, 0, 42 + i, TAMPI_COMM_WORLD, &request[i]);
    }
  }

  int flag = false;
  while(false == flag) {
    medi::TAMPI_Testall(10, request, &flag, TAMPI_STATUSES_IGNORE);
  }
}
