###############
# Build tests
###############

################
# Add tests to ctest
################

ADD_EXECUTABLE(filo_test test_filo.c)
TARGET_LINK_LIBRARIES(filo_test ${SPATH_EXTERNAL_LIBS} filo)
#ADD_TEST(NAME filo_test COMMAND ${MPIEXEC} -n3 -m block ./filo_test)
ADD_TEST(NAME filo_test COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 3 ./filo_test)


####################
# make a verbose "test" target named "check"
####################

ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
