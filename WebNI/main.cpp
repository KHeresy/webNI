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

// application Header
#include "ConnectionState.h"
#include "NIModule.h"

#pragma endregion

#pragma region Some type defination
typedef websocketpp::config::asio														TConfig;
typedef websocketpp::server<TConfig>													TServer;
typedef websocketpp::endpoint<websocketpp::connection<TConfig>,TConfig>::connection_ptr	TConnection;
#pragma endregion

#pragma region Global Objects
NIModule	g_NIModule;

// WebSocket++ Objects
TServer*	g_pServer = NULL;

// Data
std::map<TConnection,ConnectionState>	g_mapConnection;
std::map<nite::UserId,nite::Skeleton>	g_UsersData;

#pragma endregion

// TODO: Don't know where to call this?
void ExitProgram()
{
	g_pServer->stop();
	delete g_pServer;
}

inline void sendTextMessage( websocketpp::connection_hdl& hdl, const std::string& rMsg )
{
	g_pServer->send( hdl, rMsg, websocketpp::frame::opcode::TEXT );
}

template<typename TCon>
inline void sendArrayData( websocketpp::connection_hdl& hdl, const TCon& rData )
{
	g_pServer->send( hdl, rData.data(), rData.size() * sizeof(rData[0]), websocketpp::frame::opcode::BINARY );
}

#pragma region Callback function of WebSocket++
void onMessage( websocketpp::connection_hdl hdl, TServer::message_ptr msg )
{
	TConnection mCon = g_pServer->get_con_from_hdl( hdl );
	auto itCon = g_mapConnection.find( mCon );
	if( itCon != g_mapConnection.end() )
	{
		if( msg->get_opcode() == websocketpp::frame::opcode::TEXT )
		{
			try
			{
				std::stringstream ssInput( msg->get_payload() );
				std::string sCmd;
				ssInput >> sCmd;
				if( sCmd == "get" )
				{
					ssInput >> sCmd;
					if( sCmd == "skeleton" )
					{
					}
					else if( sCmd == "depth_size" )
					{
						sendArrayData( hdl, g_NIModule.getDepthSize() );
					}
					else if( sCmd == "user_list" )
					{
						auto aList = g_NIModule.getUserList();
						if( aList.size() == 0 )
							aList.push_back(0);
						sendArrayData( hdl, aList );
					}
					else if( sCmd == "user" )
					{
						uint16_t uID;
						ssInput >> uID;

						ssInput >> sCmd;
						if( sCmd == "skeleton" )
						{
							if( g_NIModule.isTracked( uID ) )
							{
								ssInput >> sCmd;
								if( sCmd == "2D" )
								{
									auto aSk = g_NIModule.getSkeleton2D(uID);
									sendArrayData( hdl, aSk );
								}
								else if( sCmd == "3D" )
								{
									auto aSk = g_NIModule.getSkeleton3D(uID);
									sendArrayData( hdl, aSk );
								}
								else
								{
									sendTextMessage( hdl, "Unknow command" );
								}
							}
							else
							{
								sendTextMessage( hdl, "The user is not tracked" );
							}
						}
					}
				}
			}
			catch( ... )
			{
				sendTextMessage( hdl, "No such command." );
			}
		}
	}
	else
	{
		sendTextMessage( hdl, "Can't found mapping connection state." );
	}
	std::cout << msg->get_payload() << std::endl;
}

void onOpen( websocketpp::connection_hdl hdl )
{
	// Insert a new data pair
	g_mapConnection.insert( std::make_pair( g_pServer->get_con_from_hdl( hdl ), ConnectionState() ) );
}

void onClose( websocketpp::connection_hdl hdl )
{
	// remove closed connection data pair
	g_mapConnection.erase( g_pServer->get_con_from_hdl( hdl ) );
}

void onTimer( const websocketpp::lib::error_code& e )
{
	g_NIModule.UpdateData();
	g_pServer->set_timer( 30, &onTimer );
}

#pragma endregion

// Main Function
int main( int argc, char** argv )
{
	// Porgram Setting
	int			iPort;
	bool		bServerLogDisplay;
	std::string	sDevice	= "";//openni::ANY_DEVICE;

	#pragma region Use boost::program_options to handle runtime setting
	{
		namespace BPO = boost::program_options;

		// define program options
		BPO::options_description bpoOptions( "Command Line Options" );
		bpoOptions.add_options()
			( "help,H",	BPO::bool_switch()->notifier( [&bpoOptions]( bool bH ){ if( bH ){ std::cout << bpoOptions << std::endl; exit(0); } } ),	"Help message" )
			( "dlog",	BPO::bool_switch(&bServerLogDisplay),									"Display WebSocket Server log" )
			( "port,P", BPO::value(&iPort)->value_name("port_num")->default_value(9002),		"The port to listen" )
			( "file,F",	BPO::value(&sDevice)->value_name("ONI_File"),							"Open an ONI file for test" );

		// prase
		try
		{
			BPO::variables_map mVMap;
			BPO::store( BPO::command_line_parser( argc, argv ).options( bpoOptions ).allow_unregistered().run(), mVMap );
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
	if( !g_NIModule.Initialize( sDevice ) )
	{
		std::cerr << " [ERROR] Can't initialize OpenNI and NiTE." << std::endl;
		return -1;
	}
	#pragma endregion

	#pragma region Initialize WebSocket++
	g_pServer = new TServer();
	if( !bServerLogDisplay )
	{
		g_pServer->set_access_channels(websocketpp::log::alevel::none);
		g_pServer->clear_access_channels(websocketpp::log::alevel::all);
	}

	// set connection callback function
	g_pServer->set_message_handler( &onMessage );
	g_pServer->set_open_handler( &onOpen );
	g_pServer->set_close_handler( &onClose );

	// initialize
	g_pServer->init_asio();
	g_pServer->listen( iPort );
	g_pServer->start_accept();

	// set timer callback functions
	g_pServer->set_timer( 30, &onTimer );

	// run
	g_pServer->run();
	#pragma endregion

	return 0;
}
