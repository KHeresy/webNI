#pragma region Header Files

// STL Header
#include <iostream>
#include <sstream>
#include <thread>

// Boost Header
#include <boost/format.hpp>
#include <boost/program_options.hpp>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

// WebSocket++ Header
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#pragma endregion

typedef websocketpp::server<websocketpp::config::asio> server;

// Callback function of WebSocket++
void on_message( websocketpp::connection_hdl hdl, server::message_ptr msg )
{
	std::cout << msg->get_payload() << std::endl;
}

// Main Function
int main( int argc, char** argv )
{
	// Porgram Setting
	int		iPort;

	#pragma region Use boost::program_options to handle runtime setting
	// TODO: parse program options
	#pragma endregion

	#pragma region Initialize OpenNI and NiTE
	// TODO: OpenNI and NiTE code
	#pragma endregion

	#pragma region Initialize WebSocket++
	server print_server;
	print_server.set_message_handler(&on_message);
	
	print_server.init_asio();
	print_server.listen(9002);
	print_server.start_accept();
	
	print_server.run();
	#pragma endregion

	return 0;
}
