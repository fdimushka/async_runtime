find_package(Boost 1.74.0 REQUIRED COMPONENTS thread chrono system regex url system context fiber)

include_directories( ${Boost_INCLUDE_DIR} )

set(BOOST_LIBRARIES Boost::headers Boost::system Boost::regex Boost::url Boost::system Boost::thread Boost::fiber)
