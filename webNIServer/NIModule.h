#pragma once

// STL Header
#include <array>
#include <map>
#include <string>
#include <vector>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

/**
 * Class to keep user data
 */
class CUserData
{
public:
	bool					m_bIsTracked;
	std::array<float,45>	m_aSkeleton2D;
	std::array<float,60>	m_aSkeleton3D;

public:
	CUserData()
	{
		m_bIsTracked = false;
	}
};

/**
 * Class to handle process of OpenNI and NiTE
 */
class NIModule
{
public:
	NIModule(){}

	~NIModule();

	/**
	 * Initialize OpenNI and NiTE
	 */
	bool Initialize( const std::string& sDevice );

	/**
	 * Update data, should be called in main loop
	 */
	void UpdateData();

	/**
	 * Get the width and height of depth map
	 */
	std::array<uint16_t,2> getDepthSize() const;

	/**
	 * Get list of users
	 */
	std::vector<uint16_t> getUserList() const;

	/**
	 * Check is the user is under tracking
	 */
	bool isTracked( nite::UserId uID )
	{
		return m_UserList[uID].m_bIsTracked;
	}

	/**
	 * Get user skeleton in depth coordinate
	 */
	const std::array<float,45>& getSkeleton2D( nite::UserId uID )
	{
		return m_UserList[uID].m_aSkeleton2D;
	}

	/**
	 * Get user skeleton in world coordinate
	 */
	const std::array<float,60>& getSkeleton3D( nite::UserId uID )
	{
		return m_UserList[uID].m_aSkeleton3D;
	}

protected:
	openni::Device				m_Device;
	openni::VideoStream			m_DepthStream;
	openni::VideoMode			m_DepthMode;
	nite::UserTracker			m_UserTracker;
	nite::UserTrackerFrameRef	m_UserFrame;

	std::map<nite::UserId,CUserData>	m_UserList;
};
