#include "NIModule.h"

// STL Header
#include <algorithm>

// Boost Header
#include <boost/format.hpp>

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

bool NIModule::Initialize( const string& sDevice, const openni::VideoMode& rMode )
{
	// initialize OpenNI
	m_funcOnInfo( "Initialize OpenNI" );
	if( OpenNI::initialize() != openni::STATUS_OK )
	{
		m_funcOnError( "Can't initialize OpenNI:\n " + string( OpenNI::getExtendedError() ) );
		return false;
	}

	// Open OpenNI Device
	m_funcOnInfo( "Open OpenNI Device" );
	if( sDevice == "" )
	{
		if( m_Device.open( openni::ANY_DEVICE ) != openni::STATUS_OK )
		{
			m_funcOnError( "Can't open OpenNI Device:\n " + string( OpenNI::getExtendedError() ) );
			return false;
		}
	}
	else
	{
		if( m_Device.open( sDevice.c_str() ) != openni::STATUS_OK )
		{
			m_funcOnError( "Can't open OpenNI Device:\n " + string( OpenNI::getExtendedError() ) );
			return false;
		}
	}

	// create depth VideoStream
	m_funcOnInfo( "Create OpenNI Depth VideoStream" );
	if( m_DepthStream.create( m_Device, SENSOR_DEPTH ) != openni::STATUS_OK )
	{
		m_funcOnError( "Can't create OpenNI Depth VideoStream:\n " + string( OpenNI::getExtendedError() ) );
		return false;
	}

	// Apply Video Mode
	if( rMode.getFps() != 0 )
	{
		m_funcOnInfo( "Set OpenNI Depth VideoStream VideoMode" );
		if( m_DepthStream.setVideoMode( rMode ) != openni::STATUS_OK )
		{
			m_funcOnError( "Can't apply OpenNI Depth VideoStream VideoMode:\n " + string( OpenNI::getExtendedError() ) );
			return false;
		}
	}
	m_DepthMode = m_DepthStream.getVideoMode();
	
	// Initialize NiTE
	m_funcOnInfo( "Initialize NiTE" );
	if( NiTE::initialize() != nite::STATUS_OK )
	{
		m_funcOnError( "Can't initialize NiTE" );
		return false;
	}

	// create UserTracker
	m_funcOnInfo( "Cretae NiTE UserTracker" );
	if( m_UserTracker.create( &m_Device ) != nite::STATUS_OK )
	{
		m_funcOnError( "Can't create NiTE User Tracker" );
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
				m_funcOnInfo( ( boost::format( "Found new user: %1%" ) % uID ).str() );
				m_UserTracker.startSkeletonTracking( uID );
				m_UserList.insert( make_pair( uID, CUserData() ) );
			}
			else if( rUser.isLost() )
			{
				m_funcOnInfo( ( boost::format( "Lost user: %1%" ) % uID ).str() );
				m_UserList.erase( uID );
			}
			else if( rUser.isVisible() )
			{
				auto& rUserData = m_UserList[uID];
				const Skeleton& rSkeleton = rUser.getSkeleton();
				if( rSkeleton.getState() == nite::SKELETON_TRACKED )
				{
					SkeletonJoint aJointList[15];
					aJointList[ 0]	= rSkeleton.getJoint( nite::JOINT_HEAD				);
					aJointList[ 1]	= rSkeleton.getJoint( nite::JOINT_NECK				);
					aJointList[ 2]	= rSkeleton.getJoint( nite::JOINT_LEFT_SHOULDER		);
					aJointList[ 3]	= rSkeleton.getJoint( nite::JOINT_RIGHT_SHOULDER	);
					aJointList[ 4]	= rSkeleton.getJoint( nite::JOINT_LEFT_ELBOW		);
					aJointList[ 5]	= rSkeleton.getJoint( nite::JOINT_RIGHT_ELBOW		);
					aJointList[ 6]	= rSkeleton.getJoint( nite::JOINT_LEFT_HAND			);
					aJointList[ 7]	= rSkeleton.getJoint( nite::JOINT_RIGHT_HAND		);
					aJointList[ 8]	= rSkeleton.getJoint( nite::JOINT_TORSO				);
					aJointList[ 9]	= rSkeleton.getJoint( nite::JOINT_LEFT_HIP			);
					aJointList[10]	= rSkeleton.getJoint( nite::JOINT_RIGHT_HIP			);
					aJointList[11]	= rSkeleton.getJoint( nite::JOINT_LEFT_KNEE			);
					aJointList[12]	= rSkeleton.getJoint( nite::JOINT_RIGHT_KNEE		);
					aJointList[13]	= rSkeleton.getJoint( nite::JOINT_LEFT_FOOT			);
					aJointList[14]	= rSkeleton.getJoint( nite::JOINT_RIGHT_FOOT		);

					#pragma omp parallel for
					for( int i = 0; i < 15; ++ i )
					{
						const Point3f& rPos = aJointList[i].getPosition();
						rUserData.m_aSkeleton3D[ i * 4     ] = rPos.x;
						rUserData.m_aSkeleton3D[ i * 4 + 1 ] = rPos.y;
						rUserData.m_aSkeleton3D[ i * 4 + 2 ] = rPos.z;
						rUserData.m_aSkeleton3D[ i * 4 + 3 ] = aJointList[i].getPositionConfidence();

						m_UserTracker.convertJointCoordinatesToDepth( rPos.x, rPos.y, rPos.z, &(rUserData.m_aSkeleton2D[ i * 3 ]), &(rUserData.m_aSkeleton2D[ i * 3 + 1 ]) );
						rUserData.m_aSkeleton3D[ i * 3 + 2 ] = aJointList[i].getPositionConfidence();
					}

					if( !rUserData.m_bIsTracked )
					{
						rUserData.m_bIsTracked = true;
						m_funcOnInfo( ( boost::format( "Strat tracking user: %1%" ) % uID ).str() );
					}
				}
				else
				{
					rUserData.m_bIsTracked = false;
				}
			}
		}
	}
}

std::vector<uint16_t> NIModule::getUserList() const
{
	std::vector<uint16_t> aList;
	for_each( m_UserList.begin(), m_UserList.end(), [&aList]( const pair<UserId,CUserData>& rUser ){
		aList.push_back( rUser.first );
	} );
	return aList;
}
