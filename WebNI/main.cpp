#pragma region Header Files

// STL Header
#include <iostream>
#include <map>
#include <sstream>

// WebSocket++ Header
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

// Boost Header
#include <boost/format.hpp>
#include <boost/program_options.hpp>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

#pragma endregion

typedef websocketpp::server<websocketpp::config::asio> server;

#pragma region Global Objects
// OpenNI and NiTE objects
openni::Device		g_Device;
nite::UserTracker	g_UserTracker;

// WebSocket++ Objects
server*	g_pServer = NULL;

// Data
std::map<nite::UserId,nite::Skeleton>	g_UsersData;

#pragma endregion

//
void ExitProgram()
{
	g_UserTracker.destroy();
	nite::NiTE::shutdown();

	g_Device.close();
	openni::OpenNI::shutdown();

	g_pServer->stop();
	delete g_pServer;
}

// Callback function of WebSocket++
void onMessage(	websocketpp::connection_hdl hdl,
				server::message_ptr msg )
{
	std::cout << msg->get_payload() << std::endl;
}

void onTimer( const websocketpp::lib::error_code& e )
{
	nite::UserTrackerFrameRef mFrame;
	if( g_UserTracker.readFrame( &mFrame ) == nite::STATUS_OK )
	{
		auto& aUsers = mFrame.getUsers();
		for( int i = 0; i < aUsers.getSize(); ++ i )
		{
			auto& rUser = aUsers[i];

			if( rUser.isNew() )
			{
				std::cout << " [NiTE] Found new user: " << rUser.getId() << std::endl;
				g_UserTracker.startSkeletonTracking( rUser.getId() );
				g_UsersData.insert( std::make_pair( rUser.getId(), nite::Skeleton() ) );
			}
			else if( rUser.isLost() )
			{
				std::cout << " [NiTE] Lost user: " << rUser.getId() << std::endl;
				g_UsersData.erase( rUser.getId() );
			}
			else
			{
				g_UsersData[rUser.getId()] = rUser.getSkeleton();
			}
		}
	}
	g_pServer->set_timer( 30, &onTimer );
}

// Main Function
int main( int argc, char** argv )
{
	// Porgram Setting
	int		iPort = 9002;

	#pragma region Use boost::program_options to handle runtime setting
	// TODO: parse program options
	#pragma endregion

	#pragma region Initialize OpenNI and NiTE
	// initialize OpenNI
	std::cout << "Initialize OpenNI" << std::endl;
	if( openni::OpenNI::initialize() != openni::STATUS_OK )
	{
		std::cerr << " [ERROR] Can't initialize OpenNI: " << openni::OpenNI::getExtendedError() << std::endl;
		return -1;
	}

	// Open OpenNI Device
	std::cout << "Open OpenNI Device" << std::endl;
	if( g_Device.open( openni::ANY_DEVICE ) != openni::STATUS_OK )
	{
		std::cerr << " [ERROR] Can't open OpenNI Device: " << openni::OpenNI::getExtendedError() << std::endl;
		return -1;
	}

	// Initialize NiTE
	std::cout << "Initialize NiTE" << std::endl;
	if( nite::NiTE::initialize() != nite::STATUS_OK )
	{
		std::cerr << " [ERROR] Can't initialize NiTE" << std::endl;
		return -1;
	}

	// create UserTracker
	std::cout << "Cretae NiTE UserTracker" << std::endl;
	if( g_UserTracker.create( &g_Device ) != nite::STATUS_OK )
	{
		std::cerr << " [ERROR] Can't create NiTE User Tracker" << std::endl;
		return -1;
	}
	#pragma endregion

	#pragma region Initialize WebSocket++
	// initialize
	g_pServer = new server();
	g_pServer->init_asio();
	g_pServer->listen( iPort );
	g_pServer->start_accept();

	// set callback functions
	g_pServer->set_message_handler( &onMessage );
	g_pServer->set_timer( 30, &onTimer );

	// run
	g_pServer->run();
	#pragma endregion

	return 0;
}
