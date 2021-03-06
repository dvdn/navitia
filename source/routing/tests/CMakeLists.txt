#We use the BOOST_LIBS define is the parent: routing
SET(BOOST_LIBS ${BOOST_LIBS} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
add_executable(raptor_test raptor_test.cpp)
target_link_libraries(raptor_test ${Boost_LIBRARIES} ed data fare routing georef
    autocomplete utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(raptor_test)

add_executable(reverse_raptor_test reverse_raptor_test.cpp)
target_link_libraries(reverse_raptor_test ed data fare routing georef autocomplete
    utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(reverse_raptor_test)

add_executable(frequency_raptor_test frequency_raptor_test.cpp)
target_link_libraries(frequency_raptor_test ed data fare routing georef autocomplete
    utils ${BOOST_LIBS} log4cplus pb_lib protobuf)
ADD_BOOST_TEST(frequency_raptor_test)

add_executable(routing_api_test routing_api_test.cpp)
target_link_libraries(routing_api_test ed data types routing fare pb_lib thermometer georef
    autocomplete utils  ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(routing_api_test)

add_executable(next_stop_time_test next_stop_time_test.cpp)
target_link_libraries(next_stop_time_test ed connectors data fare types routing
    autocomplete pb_lib thermometer autocomplete georef utils ${BOOST_LIBS} log4cplus protobuf)
ADD_BOOST_TEST(next_stop_time_test)


add_executable(raptor_odt_test raptor_odt_test.cpp)
target_link_libraries(raptor_odt_test ed data types routing fare pb_lib thermometer georef
    autocomplete utils ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(raptor_odt_test)

add_executable(disruptions_test disruptions_test.cpp)
target_link_libraries(disruptions_test ed data types routing fare pb_lib thermometer georef
  autocomplete utils workers
  ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(disruptions_test)

add_executable(co2_emission_test co2_emission_test.cpp)
target_link_libraries(co2_emission_test ed data types routing fare pb_lib thermometer georef
  autocomplete utils workers
  ${BOOST_LIBS} log4cplus pthread protobuf)
ADD_BOOST_TEST(co2_emission_test)
