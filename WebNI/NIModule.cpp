#include "NIModule.h"

// STL Header
#include <iostream>

// namespace
using namespace std;
using namespace openni;
using namespace nite;

NIModule::~NIModule()
{
	m_UserTracker.destroy();
	nite::NiTE::shutdown();

	m_DepthStream.destroy();
	m_Device.close();
	openni::OpenNI::shutdown();
}

bool NIModule::Initialize( const char* sDevice )
{
	// initialize OpenNI
	cout << "Initialize OpenNI" << endl;
	if( OpenNI::initialize() != openni::STATUS_OK )
	{
		cerr << " [ERROR] Can't initialize OpenNI: " << OpenNI::getExtendedError() << endl;
		return false;
	}

	// Open OpenNI Device
	cout << "Open OpenNI Device" << endl;
	if( m_Device.open( sDevice ) != openni::STATUS_OK )
	{
		cerr << " [ERROR] Can't open OpenNI Device: " << OpenNI::getExtendedError() << endl;
		return false;
	}

	// create depth VideoStream
	cout << "Create OpenNI Depth VideoStream" << endl;
	if( m_DepthStream.create( m_Device, SENSOR_DEPTH ) != openni::STATUS_OK )
	{
		cerr << " [ERROR] Can't create OpenNI Depth VideoStream: " << OpenNI::getExtendedError() << endl;
		return false;
	}
	m_DepthMode = m_DepthStream.getVideoMode();

	// Initialize NiTE
	cout << "Initialize NiTE" << endl;
	if( NiTE::initialize() != nite::STATUS_OK )
	{
		cerr << " [ERROR] Can't initialize NiTE" << endl;
		return false;
	}

	// create UserTracker
	cout << "Cretae NiTE UserTracker" << endl;
	if( m_UserTracker.create( &m_Device ) != nite::STATUS_OK )
	{
		cerr << " [ERROR] Can't create NiTE User Tracker" << endl;
		return false;
	}
	return true;
}

void NIModule::UpdateData()
{
	UserTrackerFrameRef mFrame;
	if( m_UserTracker.readFrame( &mFrame ) == nite::STATUS_OK )
	{
		auto& aUsers = mFrame.getUsers();
		for( int i = 0; i < aUsers.getSize(); ++ i )
		{
			auto& rUser = aUsers[i];
			const UserId& uID = rUser.getId();

			if( rUser.isNew() )
			{
				std::cout << " [NiTE] Found new user: " << uID << std::endl;
				m_UserTracker.startSkeletonTracking( uID );
			}
			else if( rUser.isLost() )
			{
				std::cout << " [NiTE] Lost user: " << uID << std::endl;
			}
			else
			{
			}
		}
	}
}

array<unsigned int,2> NIModule::getDepthSize() const
{
	array<unsigned int,2> aSize = { m_DepthMode.getResolutionX(), m_DepthMode.getResolutionY() };
	return aSize;
}
