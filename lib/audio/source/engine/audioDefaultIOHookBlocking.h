//////////////////////////////////////////////////////////////////////
//
// AkDefaultIOHookBlocking.h
//
// Default blocking low level IO hook (AK::StreamMgr::IAkIOHookBlocking) 
// and file system (AK::StreamMgr::IAkFileLocationResolver) implementation 
// on OS X. It can be used as a standalone implementation of the 
// Low-Level I/O system.
// 
// AK::StreamMgr::IAkFileLocationResolver: 
// Resolves file location using simple path concatenation logic 
// (implemented in ../Common/AudioFileLocationBase). It can be used as a 
// standalone Low-Level IO system, or as part of a multi device system. 
// In the latter case, you should manage multiple devices by implementing 
// AK::StreamMgr::IAkFileLocationResolver elsewhere (you may take a look 
// at class CAkDefaultLowLevelIODispatcher).
//
// AK::StreamMgr::IAkIOHookBlocking: 
// Uses the C Standard Input and Output Library.
// The AK::StreamMgr::IAkIOHookBlocking interface is meant to be used with
// AK_SCHEDULER_BLOCKING streaming devices. 
//
// Init() creates a streaming device (by calling AK::StreamMgr::CreateDevice()).
// AkDeviceSettings::uSchedulerTypeFlags is set inside to AK_SCHEDULER_BLOCKING.
// If there was no AK::StreamMgr::IAkFileLocationResolver previously registered 
// to the Stream Manager, this object registers itself as the File Location Resolver.
//
// Examples of streaming initialization:
// 
// Standalone (registered as the one and only File Location Resolver):
/* 
	// Create Stream Manager.
	AkStreamMgrSettings stmSettings;
	AK::StreamMgr::GetDefaultSettings( stmSettings );
	AK:IAkStreamMgr * pStreamMgr = AK::StreamMgr::Create( stmSettings );
	AKASSERT( pStreamMgr );

	// Create blocking device.
	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings( deviceSettings );
	AudioDefaultIOHookBlocking hookIOBlocking;
	AKRESULT eResult = hookIOBlocking.Init( deviceSettings );
	AKASSERT( AK_SUCCESS == eResult );
*/
//
// As part of a system with multiple devices (the File Location Resolver is 
// implemented by CAkDefaultLowLevelIODispatcher):
/* 
	// Create Stream Manager.
	AkStreamMgrSettings stmSettings;
	AK::StreamMgr::GetDefaultSettings( stmSettings );
	AK:IAkStreamMgr * pStreamMgr = AK::StreamMgr::Create( stmSettings );
	AKASSERT( pStreamMgr );

	// Create and register the File Location Resolver.
	CAkDefaultLowLevelIODispatcher lowLevelIODispatcher;
	AK::StreamMgr::SetFileLocationResolver( &lowLevelIODispatcher );

	// Create blocking device.
	AkDeviceSettings deviceSettings;
	AK::StreamMgr::GetDefaultDeviceSettings( deviceSettings );
	AudioDefaultIOHookBlocking hookIOBlocking;
	AKRESULT eResult = hookIOBlocking.Init( deviceSettings );
	AKASSERT( AK_SUCCESS == eResult );

	// Add it to the global File Location Resolver.
	lowLevelIODispatcher.AddDevice( hookIOBlocking );

	// Create more devices.
	// ...
*/
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_DEFAULT_IO_HOOK_BLOCKING_H_
#define _AK_DEFAULT_IO_HOOK_BLOCKING_H_

#include <AK/SoundEngine/Common/AkStreamMgrModule.h>
#include "audioFileHelpers.h"

//-----------------------------------------------------------------------------
// Name: class AudioDefaultIOHookBlocking.
// Desc: Implements IAkIOHookBlocking low-level I/O hook, and 
//		 IAkFileLocationResolver. Can be used as a standalone Low-Level I/O
//		 system, or as part of a system with multiple devices.
//		 File location is resolved using simple path concatenation logic
//		 (implemented in AudioFileLocationBase).
//-----------------------------------------------------------------------------
class AudioDefaultIOHookBlocking : public AK::StreamMgr::IAkFileLocationResolver
								,public AK::StreamMgr::IAkIOHookBlocking
								,public AudioMultipleFileLocation<AudioFileHelpers>
{
public:

	AudioDefaultIOHookBlocking();
	virtual ~AudioDefaultIOHookBlocking();

	// Initialization/termination. Init() registers this object as the one and 
	// only File Location Resolver if none were registered before. Then 
	// it creates a streaming device with scheduler type AK_SCHEDULER_BLOCKING.
	AKRESULT Init(
		const AkDeviceSettings &	in_deviceSettings,	// Device settings.
		bool						in_bAsyncOpen=false	// If true, files are opened asynchronously when possible.
		);
	void Term();


	//
	// IAkFileLocationAware interface.
	//-----------------------------------------------------------------------------

	// Returns a file descriptor for a given file name (string).
    virtual AKRESULT Open( 
        const AkOSChar*			in_pszFileName,		// File name.
		AkOpenMode				in_eOpenMode,		// Open mode.
        AkFileSystemFlags *		in_pFlags,			// Special flags. Can pass NULL.
		bool &					io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
        AkFileDesc &			out_fileDesc        // Returned file descriptor.
        );

    // Returns a file descriptor for a given file ID.
    virtual AKRESULT Open( 
        AkFileID				in_fileID,          // File ID.
        AkOpenMode				in_eOpenMode,       // Open mode.
        AkFileSystemFlags *		in_pFlags,			// Special flags. Can pass NULL.
		bool &					io_bSyncOpen,		// If true, the file must be opened synchronously. Otherwise it is left at the File Location Resolver's discretion. Return false if Open needs to be deferred.
        AkFileDesc &			out_fileDesc        // Returned file descriptor.
        );


	//
	// IAkIOHookBlocking interface.
	//-----------------------------------------------------------------------------

	// Reads data from a file (synchronous). 
	virtual AKRESULT Read(
        AkFileDesc &			in_fileDesc,        // File descriptor.
		const AkIoHeuristics &	in_heuristics,		// Heuristics for this data transfer.
        void *					out_pBuffer,        // Buffer to be filled with data.
        AkIOTransferInfo &		io_transferInfo		// Synchronous data transfer info. 
        );

    // Writes data to a file (synchronous). 
	virtual AKRESULT Write(
		AkFileDesc &			in_fileDesc,        // File descriptor.
		const AkIoHeuristics &	in_heuristics,		// Heuristics for this data transfer.
        void *					in_pData,           // Data to be written.
        AkIOTransferInfo &		io_transferInfo		// Synchronous data transfer info. 
        );

	// Cleans up a file.
    virtual AKRESULT Close(
        AkFileDesc &			in_fileDesc			// File descriptor.
        );

	// Returns the block size for the file or its storage device. 
	virtual AkUInt32 GetBlockSize(
        AkFileDesc &  			in_fileDesc			// File descriptor.
        );

	// Returns a description for the streaming device above this low-level hook.
    virtual void GetDeviceDesc(
        AkDeviceDesc &  		out_deviceDesc      // Device description.
        );
		
	// Returns custom profiling data: 1 if file opens are asynchronous, 0 otherwise.
	virtual AkUInt32 GetDeviceData();

protected:
	AkDeviceID	m_deviceID;
	bool		m_bAsyncOpen;	// If true, opens files asynchronously when it can.
};

#endif //_AK_DEFAULT_IO_HOOK_BLOCKING_H_
