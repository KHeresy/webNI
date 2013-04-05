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

// application Header
#include "ConnectionState.h"

#pragma endregion

typedef websocketpp::config::asio														TConfig;
typedef websocketpp::server<TConfig>													TServer;
typedef websocketpp::endpoint<websocketpp::connection<TConfig>,TConfig>::connection_ptr	TConnection;

#pragma region Global Objects
// OpenNI and NiTE objects
openni::Device		g_Device;
nite::UserTracker	g_UserTracker;

// WebSocket++ Objects
TServer*	g_pServer = NULL;

// Data
std::map<TConnection,ConnectionState>	g_mapConnection;
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
				TServer::message_ptr msg )
{
	TConnection mCon = g_pServer->get_con_from_hdl( hdl );
	auto itCon = g_mapConnection.find( mCon );
	if( itCon != g_mapConnection.end() )
	{
		
	}
	else
	{
		g_pServer->send( hdl, "Can't found mapping connection state.", websocketpp::frame::opcode::TEXT );
	}
	g_pServer->send( hdl, "Can't found mapping connection state.", websocketpp::frame::opcode::TEXT );
	std::cout << msg->get_payload() << std::endl;
}

void onOpen( websocketpp::connection_hdl hdl )
{
	g_mapConnection.insert( std::make_pair( g_pServer->get_con_from_hdl( hdl ), ConnectionState() ) );
}

void onClose( websocketpp::connection_hdl hdl )
{
	g_mapConnection.erase( g_pServer->get_con_from_hdl( hdl ) );
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
	int			iPort;
	std::string	sDevice	= "";//openni::ANY_DEVICE;

	#pragma region Use boost::program_options to handle runtime setting
	{
		namespace BPO = boost::program_options;

		// define program options
		BPO::options_description bpoOptions( "Command Line Options" );
		bpoOptions.add_options()
			( "help,H",	BPO::bool_switch()->notifier( [&bpoOptions]( bool bH ){ if( bH ){ std::cout << bpoOptions << std::endl; exit(0); } } ),	"Help message" )
			( "port,P", BPO::value(&iPort)->value_name("port_num")->default_value(9002),		"The port to listen" )
			( "file,F",	BPO::value(&sDevice)->value_name("ONI_File"),							"Open an ONI file for test" );

		// prase
		try
		{
			BPO::variables_map mVMap;
			BPO::store( BPO::parse_command_line( argc, argv, bpoOptions ), mVMap );
			BPO::notify( mVMap );
		}
		catch( BPO::error_with_option_name e )
		{
			std::cerr << e.what() << std::endl;
			std::cout << bpoOptions << std::endl;
			return 1;
		}
		catch( BPO::error e )
		{
			std::cerr << e.what() << std::endl;
			std::cout << bpoOptions << std::endl;
			return 1;
		}
	}
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
	if( g_Device.open( sDevice.c_str() ) != openni::STATUS_OK )
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
	g_pServer = new TServer();
	g_pServer->init_asio();
	g_pServer->listen( iPort );
	g_pServer->start_accept();

	// set callback functions
	g_pServer->set_message_handler( &onMessage );
	g_pServer->set_open_handler( &onOpen );
	g_pServer->set_close_handler( &onClose );
	g_pServer->set_timer( 30, &onTimer );

	// run
	g_pServer->run();
	#pragma endregion

	return 0;
}
