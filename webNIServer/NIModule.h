/**
 * This file is the NIModule of webNI server.
 *				version 0.3 @2013/05/01
 *
 * This server is developed by Heresy
 * The full project: https://github.com/KHeresy/webNI
 * My blog: http://kheresy.wordpress.com/
 */

#pragma once

#pragma region Header Files
// STL Header
#include <array>
#include <map>
#include <string>
#include <vector>

// Boost Header
#include <boost/signals2.hpp>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

#pragma endregion

/**
 * Class to keep user data
 */
class CUserData
{
public:
	enum EStatus
	{
		USER_NONE		= 0,
		USER_NEW		= 1,
		USER_NORMAL		= 2,
		USER_TRACKED	= 3,
		USER_INVISIBLE	= 4
	};

public:
	EStatus					m_eStatus;
	std::array<float,45>	m_aSkeleton2D;
	std::array<float,60>	m_aSkeleton3D;

public:
	CUserData()
	{
		m_eStatus = USER_NEW;
	}
};

/**
 * Class to handle process of OpenNI and NiTE
 */
class NIModule
{
public:
	boost::signals2::signal<void(std::string)>	m_funcOnError;	/**< signal emit on error */
	boost::signals2::signal<void(std::string)>	m_funcOnInfo;	/**< signal emit for information */

public:
	NIModule(){}

	~NIModule();

	/**
	 * Initialize OpenNI and NiTE
	 */
	bool Initialize( const std::string& sDevice, const openni::VideoMode& rMode );

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
	bool isTracked( const nite::UserId uID ) const
	{
		return ( m_UserList.at( uID ).m_eStatus == CUserData::USER_TRACKED );
	}

	/**
	 * Get user skeleton in depth coordinate
	 */
	const std::array<float,45>& getSkeleton2D( const nite::UserId uID ) const;

	/**
	 * Get user skeleton in world coordinate
	 */
	const std::array<float,60>& getSkeleton3D( const nite::UserId uID ) const;

	/**
	 * get a single joint position of skeleton
	 */
	const std::array<float,3> getJoint2D( const nite::UserId uID, const std::string& rJointName ) const;

	/**
	 * get a single joint position of skeleton
	 */
	const std::array<float,4> getJoint3D( const nite::UserId uID, const std::string& rJointName ) const;

protected:
	openni::Device				m_Device;
	openni::VideoStream			m_DepthStream;
	openni::VideoMode			m_DepthMode;
	nite::UserTracker			m_UserTracker;
	nite::UserTrackerFrameRef	m_UserFrame;

	std::map<nite::UserId,CUserData>	m_UserList;

protected:
	static std::map<std::string,size_t>	s_JointIdx;

	static std::map<std::string,size_t> BuildJointTable();
};

#pragma region inline functions
inline const std::array<float,45>& NIModule::getSkeleton2D( const nite::UserId uID ) const
{
	return m_UserList.at( uID ).m_aSkeleton2D;
}

inline const std::array<float,60>& NIModule::getSkeleton3D( const nite::UserId uID ) const
{
	return m_UserList.at( uID ).m_aSkeleton3D;
}

inline const std::array<float,3> NIModule::getJoint2D( const nite::UserId uID, const std::string& rJointName ) const
{
	try
	{
		size_t uIdx = s_JointIdx.at( rJointName );
		auto itData = m_UserList.at( uID ).m_aSkeleton2D.begin() + 3 * uIdx;
		std::array<float,3> aJoint = { *itData, *(++itData), *(++itData) };
		return aJoint;
	}
	catch(...)
	{
		std::array<float,3> aEmpty = {0,0,0};
		return aEmpty;
	}
}

inline const std::array<float,4> NIModule::getJoint3D( const nite::UserId uID, const std::string& rJointName ) const
{
	try
	{
		size_t uIdx = s_JointIdx.at( rJointName );
		auto itData = m_UserList.at( uID ).m_aSkeleton2D.begin() + 4 * uIdx;
		std::array<float,4> aJoint = { *itData, *(++itData), *(++itData), *(++itData) };
		return aJoint;
	}
	catch(...)
	{
		std::array<float,4> aEmpty = {0,0,0,0};
		return aEmpty;
	}
}

inline std::array<uint16_t,2> NIModule::getDepthSize() const
{
	std::array<uint16_t,2> aSize = { uint16_t(m_DepthMode.getResolutionX()), uint16_t(m_DepthMode.getResolutionY()) };
	return aSize;
}

#pragma endregion

