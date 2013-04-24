#include "webNIClient.h"

// STL Header
#include <iostream>
#include <map>
#include <sstream>

int main( int argc, char** argv )
{
	webNI::Client mClient;
	mClient.Open( "ws://localhost:9002" );

	try
	{
		auto a = mClient.GetUserList();
	}
	catch( std::exception e )
	{
		std::cerr << e.what() << std::endl;
	}

	return 0;
}
