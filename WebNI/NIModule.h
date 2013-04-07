#pragma once

// STL Header
#include <array>

// OpenNI and NiTE Header
#include <OpenNI.h>
#include <NiTE.h>

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
	bool Initialize( const char* sDevice );

	/**
	 * Update data, should be called in main loop
	 */
	void UpdateData();

	/**
	 * Get the width and height of depth map
	 */
	std::array<uint16_t,2> getDepthSize() const;

	unsigned int getUserNum() const;

protected:
	openni::Device				m_Device;
	openni::VideoStream			m_DepthStream;
	openni::VideoMode			m_DepthMode;
	nite::UserTracker			m_UserTracker;
	nite::UserTrackerFrameRef	m_UserFrame;
};
