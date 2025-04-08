find_package(Boost 1.81.0 REQUIRED COMPONENTS thread chrono system regex system context)

include_directories( ${Boost_INCLUDE_DIR} )

set(BOOST_LIBRARIES Boost::headers Boost::system Boost::regex Boost::thread Boost::context Boost::chrono)
