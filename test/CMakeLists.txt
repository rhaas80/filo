###############
# Build tests
###############

################
# Add tests to ctest
################

ADD_EXECUTABLE(filo_test test_filo.c)
TARGET_LINK_LIBRARIES(filo_test ${SPATH_EXTERNAL_LIBS} filo)
FILO_ADD_TEST(filo_test 256 "")
#ADD_TEST(NAME filo_test COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 3 ./filo_test)

ADD_EXECUTABLE(filo_corrupt_fetch_test test_filo_corrupt_fetch.c)
TARGET_LINK_LIBRARIES(filo_corrupt_fetch_test ${SPATH_EXTERNAL_LIBS} filo)
FILO_ADD_TEST(filo_corrupt_fetch_test 256 "")
#ADD_TEST(NAME filo_corrupt_fetch_test COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 3 ./filo_corrupt_fetch_test)

ADD_EXECUTABLE(filo_async_test test_filo_async.c)
TARGET_LINK_LIBRARIES(filo_async_test ${SPATH_EXTERNAL_LIBS} filo)
FILO_ADD_TEST(filo_async_test 256 "")
#ADD_TEST(NAME filo_async_test COMMAND ${MPIEXEC} ${MPIEXEC_NUMPROC_FLAG} 3 ./filo_async_test)

####################
# make a verbose "test" target named "check"
####################

ADD_CUSTOM_TARGET(check COMMAND ${CMAKE_CTEST_COMMAND} --verbose)
