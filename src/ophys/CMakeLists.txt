include_directories(
  ${BULLET_INCLUDE_DIR}
  ${EIGEN3_INCLUDE_DIR} 
  ${BULLETSIM_SOURCE_DIR}/src
  ${OpenRAVE_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
  ${GUROBI_INCLUDE_DIR}
)

set(CMAKE_CXX_FLAGS "-Wall -Wno-sign-compare -Wno-reorder -fopenmp -march=native")
link_directories(${GUROBI_LIBRARY_DIR} /home/jonathan/opt/pardiso)

add_library(splines
  hermite_cubic.cpp
  splines.cpp
)

set(IPOPT_LIBS
  #/home/jonathan/Downloads/Ipopt-3.10.3/build/Ipopt/src/Interfaces/.libs/libipopt.a
  ipopt
  gfortran pthread m pardiso lapack blas dl
)
set(CMAKE_CXX_FLAGS "-Wl,--no-as-needed -lprofiler -Wl,--as-needed")
add_library(ophys
  #particlesystem2.cpp
  ophys_config.cpp
  optrope.cpp
  optrope_state.cpp
  pr2_fk.cpp
)
target_link_libraries(ophys simulation robots nlopt splines ${IPOPT_LIBS})

#add_executable(test_particlesystem test_particlesystem.cpp)
#target_link_libraries(test_particlesystem ophys)

#add_executable(test_ropesystem test_ropesystem.cpp)
#target_link_libraries(test_ropesystem ophys)

#add_executable(test_particlesystem2 test_particlesystem2.cpp)
#target_link_libraries(test_particlesystem2 ophys)

#add_executable(test_ropesystem2 test_ropesystem2.cpp)
#target_link_libraries(test_ropesystem2 ophys)

add_executable(test_gurobi_constraints test_gurobi_constraints.cpp)
target_link_libraries(test_gurobi_constraints ophys)

add_executable(nlopt_spline_rope nlopt_spline_rope.cpp)
target_link_libraries(nlopt_spline_rope ophys)

add_executable(optrope_viewer optrope_viewer.cpp)
target_link_libraries(optrope_viewer ophys)

add_executable(test_ipopt test_ipopt.cpp)
target_link_libraries(test_ipopt ${IPOPT_LIBS})

add_executable(test_nlopt_boxconstrained test_nlopt_boxconstrained.cpp)
target_link_libraries(test_nlopt_boxconstrained nlopt)

add_executable(test_splines test_splines.cpp)
target_link_libraries(test_splines splines)

add_executable(test_pr2_fk test_pr2_fk.cpp)
target_link_libraries(test_pr2_fk simulation robots ophys)
