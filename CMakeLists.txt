cmake_minimum_required(VERSION 3.19)
project(semestralka)

set(CMAKE_CXX_STANDARD 11)
set(GCC_COVERAGE_COMPILE_FLAGS "-Wall -pedantic -lm -O3 -funroll-loops")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

add_executable(semestralka src/main.cpp)

find_package(OpenMP)
target_link_libraries(semestralka PUBLIC OpenMP::OpenMP_CXX)

set(MPIEXEC_EXECUTABLE /usr/lib64/openmpi/bin/mpicxx)
enable_language(Fortran)
find_package(MPI REQUIRED)
target_link_libraries(semestralka PUBLIC MPI::MPI_CXX)

message(STATUS "Run: ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} ${MPIEXEC_MAX_NUMPROCS} ${MPIEXEC_PREFLAGS} EXECUTABLE ${MPIEXEC_POSTFLAGS} ARGS")
