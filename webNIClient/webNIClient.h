/**
 * This file is the C++Client of webNI
 *				version 0.3 @2013/04/24
 *
 * This server is developed by Heresy
 * The full project: https://github.com/KHeresy/webNI
 * My blog: http://kheresy.wordpress.com/
 */

#pragma once

#pragma region Header Files

// STL Header
#include <string>
#include <vector>

// Boost Header
#include <boost/format.hpp>
#include <boost/signals2.hpp>

// WebSocket++ Header
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#pragma endregion

namespace webNI
{
	struct UserData
	{
		uint16_t	uID;
		bool		bTracked;
	};

	struct Joint2D
	{
		float x;
		float y;
		float c;
	};

	struct Joint3D
	{
		float x;
		float y;
		float z;
		float c;
	};

	class Client
	{
	public:
		boost::signals2::signal<void(std::string)>	m_funcOnError;	/**< signal emit on error */
		boost::signals2::signal<void(std::string)>	m_funcOnInfo;	/**< signal emit for information */

	public:
		Client();

		~Client();

		bool Open( const std::string& rURI );

		std::vector<UserData> GetUserList();

	#pragma region Some internal type defination
	protected:
		typedef websocketpp::config::asio_client					TClientConfig;
		typedef websocketpp::client<TClientConfig>					TCleint;
		typedef websocketpp::config::asio_client::message_type::ptr TMessage;
	#pragma endregion

	protected:
		bool						m_bOpen;
		std::string					m_sURI;
		TCleint						m_Client;
		websocketpp::connection_hdl	m_hdl;

	protected:
		void webSocketCommand()
		{
		}
	};
}

#pragma region Inline functions
webNI::Client::Client()
{
	m_bOpen = false;
	m_Client.init_asio();
	
	m_Client.clear_access_channels( websocketpp::log::alevel::all );
	m_Client.clear_error_channels( websocketpp::log::elevel::all );
}

webNI::Client::~Client()
{
}

bool webNI::Client::Open( const std::string& rURI )
{
	m_sURI = rURI;

	bool& bOpen = m_bOpen;
	m_Client.set_open_handler( [&bOpen]( websocketpp::connection_hdl hdl ){
		bOpen = true;
	});
	m_Client.set_close_handler( [&bOpen]( websocketpp::connection_hdl hdl ){
		bOpen = false;
	});

	m_funcOnInfo( "Try connect to " + rURI );
	websocketpp::lib::error_code ec;
	TCleint::connection_ptr mConnection = m_Client.get_connection( m_sURI, ec );
	if( ec )
	{
		m_funcOnError( ec.message() );
		return false;
	}
	m_hdl = mConnection->get_handle();
	m_Client.connect( mConnection );

	websocketpp::lib::thread asio_thread( &TCleint::run, &m_Client );

	while( !m_bOpen )
	{
	}
	m_funcOnInfo( "Connect OK" );

	return true;
}

std::vector<webNI::UserData> webNI::Client::GetUserList()
{
	std::vector<webNI::UserData> vList;

	bool bDone = false;
	m_Client.set_message_handler( [ &vList, &bDone ]( websocketpp::connection_hdl hdl, TMessage msg ){
		if( msg->get_opcode() == websocketpp::frame::opcode::TEXT )
		{
		}
		else
		{
			const std::string& sRawData = msg->get_payload();
			const uint16_t* pData = reinterpret_cast<const uint16_t*>( sRawData.data() );
			size_t uSize = sRawData.size() / sizeof( uint16_t );

			if( uSize > 1 )
			{
				for( int i = 0; i < uSize; i += 2 )
				{
					UserData mUD;
					mUD.uID = pData[ 2 * i ];
					if( pData[ 2 * i + 1 ] == 3 )
						mUD.bTracked = true;
					else
						mUD.bTracked = false;

					vList.push_back( mUD );
				}
			}
		}
		bDone = true;
	} );
	m_Client.send( m_hdl, "get userlist", websocketpp::frame::opcode::text );

	while( !bDone )
	{
	}
	return vList;
}

#pragma endregion
