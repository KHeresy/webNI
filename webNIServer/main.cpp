#pragma region Program define
#define VERSION "0.0.1"
#pragma endregion

#pragma region Header Files

// STL Header
#include <iostream>
#include <map>
#include <sstream>

// WebSocket++ Header
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

// Boost Header
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/lexical_cast.hpp>
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

void OutputServerCommand( std::ostream& rOutStream )
{
	rOutStream << "get depth_list" <<"\n";
	rOutStream << "get user_list" <<"\n";
	rOutStream << "get user <id> skeleton 2D" <<"\n";
	rOutStream << "get user <id> skeleton 3D" <<"\n";
	rOutStream << "get user <id> joint <joint_name> 2D" <<"\n";
	rOutStream << "get user <id> joint <joint_name> 3D" <<"\n";
}

#pragma region inline functions
inline void sendTextMessage( websocketpp::connection_hdl& hdl, const std::string& rMsg )
{
	g_pServer->send( hdl, rMsg, websocketpp::frame::opcode::TEXT );
}

template<typename TCon>
inline void sendArrayData( websocketpp::connection_hdl& hdl, const TCon& rData )
{
	g_pServer->send( hdl, rData.data(), rData.size() * sizeof(rData[0]), websocketpp::frame::opcode::BINARY );
}

std::istream& operator>>( std::istream& rInput, openni::VideoMode& rMode )
{
	std::string sData;
	rInput >> sData;
	std::vector<std::string> vData;
	boost::split( vData, sData, boost::is_any_of( "/@" ), boost::token_compress_on );
	if( vData.size() != 3 )
	{
		throw std::exception( "video_mode data format error\n. example: 640/480@30" );
	}
	else
	{
		rMode.setResolution( boost::lexical_cast<int>( vData[0] ), boost::lexical_cast<int>( vData[1] ) );
		rMode.setFps( boost::lexical_cast<int>( vData[2] ) );
		rMode.setPixelFormat( openni::PIXEL_FORMAT_DEPTH_1_MM );
	}
	return rInput;
}

#pragma endregion

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
						else if( sCmd == "joint" )
						{
							if( g_NIModule.isTracked( uID ) )
							{
								std::string sJointName;
								ssInput >> sJointName;

								ssInput >> sCmd;
								if( sCmd == "2D" )
								{
									auto aJoint = g_NIModule.getJoint2D( uID, sJointName );
									sendArrayData( hdl, aJoint );
								}
								else if( sCmd == "3D" )
								{
									auto aJoint = g_NIModule.getJoint3D( uID, sJointName );
									sendArrayData( hdl, aJoint );
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
	int					iPort;
	bool				bServerLogDisplay;
	std::string			sDevice	= "";//openni::ANY_DEVICE;
	openni::VideoMode	mDepthMode;
	mDepthMode.setFps( 0 );

	#pragma region Use boost::program_options to handle runtime setting
	{
		namespace BPO = boost::program_options;

		// define program options
		BPO::options_description bpoOptions( "Command Line Options" );
		bpoOptions.add_options()
			( "help,H",		BPO::bool_switch()->notifier( [&bpoOptions]( bool bH ){ if( bH ){ std::cout << bpoOptions << std::endl; exit(0); } } ),	"Help message" )
			( "server_log",	BPO::bool_switch(&bServerLogDisplay),								"Display WebSocket Server log" )
			( "port,P",		BPO::value(&iPort)->value_name("port_num")->default_value(9002),	"The port to listen" )
			( "file,F",		BPO::value(&sDevice)->value_name("ONI_File"),						"Open an ONI file for test" )
			( "video_mode",	BPO::value(&mDepthMode)->value_name("width/height@FPS"),			"VideoMode of depth" );

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
		catch(std::exception e)
		{
			std::cerr << e.what() << std::endl;
			std::cout << bpoOptions << std::endl;
			return 2;
		}
	}
	#pragma endregion

	#pragma region Initialize OpenNI and NiTE
	g_NIModule.m_funcOnError.connect( [](std::string sMsg){ std::cerr << "[ERROR][NIModule]" << sMsg << std::endl; } );
	g_NIModule.m_funcOnInfo.connect( [](std::string sMsg){ std::cout << "[INFO][NIModule]" << sMsg << std::endl; } );
	if( !g_NIModule.Initialize( sDevice, mDepthMode ) )
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
