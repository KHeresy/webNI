#pragma region Header Files

// STL Header
#include <iostream>
#include <thread>

// Boost Header
#include <boost/program_options.hpp>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

// WebSocket++ Header
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#pragma endregion

typedef websocketpp::server<websocketpp::config::asio> server;

void on_message( websocketpp::connection_hdl hdl, server::message_ptr msg )
{
	std::cout << msg->get_payload() << std::endl;
}

// Main Function
int main( int argc, char** argv )
{
	server print_server;
	print_server.set_message_handler(&on_message);
	
	print_server.init_asio();
	print_server.listen(9002);
	print_server.start_accept();
	
	print_server.run();

	return 0;
}
