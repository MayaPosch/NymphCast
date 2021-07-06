#include "VolumeControl.h"

#include "math/Misc.h"
#include "Log.h"
#include "Settings.h"
#ifdef WIN32
#include <mmdeviceapi.h>
#endif

#if defined(__linux__)
    #if defined(_RPI_) || defined(_VERO4K_)
        const char * VolumeControl::mixerName = "PCM";
    #else
    	const char * VolumeControl::mixerName = "Master";
    #endif
    const char * VolumeControl::mixerCard = "default";
#endif

std::weak_ptr<VolumeControl> VolumeControl::sInstance;


VolumeControl::VolumeControl()
	: originalVolume(0), internalVolume(0)
#if defined (__APPLE__)
	#error TODO: Not implemented for MacOS yet!!!
#elif defined(__linux__)
	, mixerIndex(0), mixerHandle(nullptr), mixerElem(nullptr), mixerSelemId(nullptr)
#elif defined(WIN32) || defined(_WIN32)
	, mixerHandle(nullptr), endpointVolume(nullptr)
#endif
{
	init();

	//get original volume levels for system
	originalVolume = getVolume();
}

VolumeControl::VolumeControl(const VolumeControl & right):
	originalVolume(0), internalVolume(0)
#if defined (__APPLE__)
	#error TODO: Not implemented for MacOS yet!!!
#elif defined(__linux__)
	, mixerIndex(0), mixerHandle(nullptr), mixerElem(nullptr), mixerSelemId(nullptr)
#elif defined(WIN32) || defined(_WIN32)
	, mixerHandle(nullptr), endpointVolume(nullptr)
#endif
{
	(void)right;
	sInstance = right.sInstance;
}

VolumeControl & VolumeControl::operator=(const VolumeControl & right)
{
	if (this != &right) {
		sInstance = right.sInstance;
	}

	return *this;
}

VolumeControl::~VolumeControl()
{
	//set original volume levels for system
	//setVolume(originalVolume);

	deinit();
}

std::shared_ptr<VolumeControl> & VolumeControl::getInstance()
{
	//check if an VolumeControl instance is already created, if not create one
	static std::shared_ptr<VolumeControl> sharedInstance = sInstance.lock();
	if (sharedInstance == nullptr) {
		sharedInstance.reset(new VolumeControl);
		sInstance = sharedInstance;
	}
	return sharedInstance;
}

void VolumeControl::init()
{
	//initialize audio mixer interface
#if defined (__APPLE__)
	#error TODO: Not implemented for MacOS yet!!!
#elif defined(__linux__)
	//try to open mixer device
	if (mixerHandle == nullptr)
	{
		// Allow users to override the AudioCard and MixerName in es_settings.cfg
		mixerCard = Settings::getInstance()->getString("AudioCard").c_str();
		mixerName = Settings::getInstance()->getString("AudioDevice").c_str();

		snd_mixer_selem_id_alloca(&mixerSelemId);
		//sets simple-mixer index and name
		snd_mixer_selem_id_set_index(mixerSelemId, mixerIndex);
		snd_mixer_selem_id_set_name(mixerSelemId, mixerName);
		//open mixer
		if (snd_mixer_open(&mixerHandle, 0) >= 0)
		{
			LOG(LogDebug) << "VolumeControl::init() - Opened ALSA mixer";
			//ok. attach to defualt card
			if (snd_mixer_attach(mixerHandle, mixerCard) >= 0)
			{
				LOG(LogDebug) << "VolumeControl::init() - Attached to default card";
				//ok. register simple element class
				if (snd_mixer_selem_register(mixerHandle, NULL, NULL) >= 0)
				{
					LOG(LogDebug) << "VolumeControl::init() - Registered simple element class";
					//ok. load registered elements
					if (snd_mixer_load(mixerHandle) >= 0)
					{
						LOG(LogDebug) << "VolumeControl::init() - Loaded mixer elements";
						//ok. find elements now
						mixerElem = snd_mixer_find_selem(mixerHandle, mixerSelemId);
						if (mixerElem != nullptr)
						{
							//wohoo. good to go...
							LOG(LogDebug) << "VolumeControl::init() - Mixer initialized";
						}
						else
						{
							LOG(LogError) << "VolumeControl::init() - Failed to find mixer elements!";
							snd_mixer_close(mixerHandle);
							mixerHandle = nullptr;
						}
					}
					else
					{
						LOG(LogError) << "VolumeControl::init() - Failed to load mixer elements!";
						snd_mixer_close(mixerHandle);
						mixerHandle = nullptr;
					}
				}
				else
				{
					LOG(LogError) << "VolumeControl::init() - Failed to register simple element class!";
					snd_mixer_close(mixerHandle);
					mixerHandle = nullptr;
				}
			}
			else
			{
				LOG(LogError) << "VolumeControl::init() - Failed to attach to default card!";
				snd_mixer_close(mixerHandle);
				mixerHandle = nullptr;
			}
		}
		else
		{
			LOG(LogError) << "VolumeControl::init() - Failed to open ALSA mixer!";
		}
	}
#elif defined(WIN32) || defined(_WIN32)
	//get windows version information
	OSVERSIONINFOEXA osVer = {sizeof(OSVERSIONINFO)};
	::GetVersionExA(reinterpret_cast<LPOSVERSIONINFOA>(&osVer));
	//check windows version
	if(osVer.dwMajorVersion < 6)
	{
		//Windows older than Vista. use mixer API. open default mixer
		if (mixerHandle == nullptr)
		{
			if (mixerOpen(&mixerHandle, 0, NULL, 0, 0) == MMSYSERR_NOERROR)
			{
				//retrieve info on the volume slider control for the "Speaker Out" line
				MIXERLINECONTROLS mixerLineControls;
				mixerLineControls.cbStruct = sizeof(MIXERLINECONTROLS);
				mixerLineControls.dwLineID = 0xFFFF0000; //Id of "Speaker Out" line
				mixerLineControls.cControls = 1;
				//mixerLineControls.dwControlID = 0x00000000; //Id of "Speaker Out" line's volume slider
				mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME; //Get volume control
				mixerLineControls.pamxctrl = &mixerControl;
				mixerLineControls.cbmxctrl = sizeof(MIXERCONTROL);
				if (mixerGetLineControls((HMIXEROBJ)mixerHandle, &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE) != MMSYSERR_NOERROR)
				{
					LOG(LogError) << "VolumeControl::init() - Failed to get mixer volume control!";
					mixerClose(mixerHandle);
					mixerHandle = nullptr;
				}
			}
			else
			{
				LOG(LogError) << "VolumeControl::init() - Failed to open mixer!";
			}
		}
	}
	else
	{
		//Windows Vista or above. use EndpointVolume API. get device enumerator
		if (endpointVolume == nullptr)
		{
			CoInitialize(nullptr);
			IMMDeviceEnumerator * deviceEnumerator = nullptr;
			CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
			if (deviceEnumerator != nullptr)
			{
				//get default endpoint
				IMMDevice * defaultDevice = nullptr;
				deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
				if (defaultDevice != nullptr)
				{
					//retrieve endpoint volume
					defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, (LPVOID *)&endpointVolume);
					if (endpointVolume == nullptr)
					{
						LOG(LogError) << "VolumeControl::init() - Failed to get default audio endpoint volume!";
					}
					//release default device. we don't need it anymore
					defaultDevice->Release();
				}
				else
				{
					LOG(LogError) << "VolumeControl::init() - Failed to get default audio endpoint!";
				}
				//release device enumerator. we don't need it anymore
				deviceEnumerator->Release();
			}
			else
			{
				LOG(LogError) << "VolumeControl::init() - Failed to get audio endpoint enumerator!";
				CoUninitialize();
			}
		}
	}
#endif
}

void VolumeControl::deinit()
{
	//deinitialize audio mixer interface
#if defined (__APPLE__)
	#error TODO: Not implemented for MacOS yet!!!
#elif defined(__linux__)
	if (mixerHandle != nullptr) {
		snd_mixer_detach(mixerHandle, mixerCard);
		snd_mixer_free(mixerHandle);
		snd_mixer_close(mixerHandle);
		mixerHandle = nullptr;
		mixerElem = nullptr;
	}
#elif defined(WIN32) || defined(_WIN32)
	if (mixerHandle != nullptr) {
		mixerClose(mixerHandle);
		mixerHandle = nullptr;
	}
	else if (endpointVolume != nullptr) {
		endpointVolume->Release();
		endpointVolume = nullptr;
		CoUninitialize();
	}
#endif
}

int VolumeControl::getVolume() const
{
	int volume = 0;

#if defined (__APPLE__)
	#error TODO: Not implemented for MacOS yet!!!
#elif defined(__linux__)
	if (mixerElem != nullptr)
	{
		//get volume range
		long minVolume;
		long maxVolume;
		if (snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume) == 0)
		{
			//ok. now get volume
			long rawVolume;
			if (snd_mixer_selem_get_playback_volume(mixerElem, SND_MIXER_SCHN_MONO, &rawVolume) == 0)
			{
				//worked. bring into range 0-100
				rawVolume -= minVolume;
				if (rawVolume > 0)
				{
					volume = (rawVolume * 100.0) / (maxVolume - minVolume) + 0.5;
				}
				//else volume = 0;
			}
			else
			{
				LOG(LogError) << "VolumeControl::getVolume() - Failed to get mixer volume!";
			}
		}
		else
		{
			LOG(LogError) << "VolumeControl::getVolume() - Failed to get volume range!";
		}
	}
#elif defined(WIN32) || defined(_WIN32)
	if (mixerHandle != nullptr)
	{
		//Windows older than Vista. use mixer API. get volume from line control
		MIXERCONTROLDETAILS_UNSIGNED value;
		MIXERCONTROLDETAILS mixerControlDetails;
		mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
		mixerControlDetails.dwControlID = mixerControl.dwControlID;
		mixerControlDetails.cChannels = 1; //always 1 for a MIXERCONTROL_CONTROLF_UNIFORM control
		mixerControlDetails.cMultipleItems = 0; //always 0 except for a MIXERCONTROL_CONTROLF_MULTIPLE control
		mixerControlDetails.paDetails = &value;
		mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
		if (mixerGetControlDetails((HMIXEROBJ)mixerHandle, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
		{
			volume = (int)Math::round((value.dwValue * 100) / 65535.0f);
		}
		else
		{
			LOG(LogError) << "VolumeControl::getVolume() - Failed to get mixer volume!";
		}
	}
	else if (endpointVolume != nullptr)
	{
		//Windows Vista or above. use EndpointVolume API
		float floatVolume = 0.0f; //0-1
		if (endpointVolume->GetMasterVolumeLevelScalar(&floatVolume) == S_OK)
		{
			volume = (int)Math::round(floatVolume * 100.0f);
			LOG(LogInfo) << " getting volume as " << volume << " ( from float " << floatVolume << ")";
		}
		else
		{
			LOG(LogError) << "VolumeControl::getVolume() - Failed to get master volume!";
		}

	}
#endif
	//clamp to 0-100 range
	if (volume < 0)
	{
		volume = 0;
	}
	if (volume > 100)
	{
		volume = 100;
	}
	return volume;
}

void VolumeControl::setVolume(int volume)
{
	//clamp to 0-100 range
	if (volume < 0)
	{
		volume = 0;
	}
	if (volume > 100)
	{
		volume = 100;
	}
	//store values in internal variables
	internalVolume = volume;
#if defined (__APPLE__)
	#error TODO: Not implemented for MacOS yet!!!
#elif defined(__linux__)
	if (mixerElem != nullptr)
	{
		//get volume range
		long minVolume;
		long maxVolume;
		if (snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume) == 0)
		{
			//ok. bring into minVolume-maxVolume range and set
			long rawVolume = (volume * (maxVolume - minVolume) / 100) + minVolume;
			if (snd_mixer_selem_set_playback_volume(mixerElem, SND_MIXER_SCHN_FRONT_LEFT, rawVolume) < 0
				|| snd_mixer_selem_set_playback_volume(mixerElem, SND_MIXER_SCHN_FRONT_RIGHT, rawVolume) < 0)
			{
				LOG(LogError) << "VolumeControl::setVolume() - Failed to set mixer volume!";
			}
		}
		else
		{
			LOG(LogError) << "VolumeControl::setVolume() - Failed to get volume range!";
		}
	}
#elif defined(WIN32) || defined(_WIN32)
	if (mixerHandle != nullptr)
	{
		//Windows older than Vista. use mixer API. get volume from line control
		MIXERCONTROLDETAILS_UNSIGNED value;
		value.dwValue = (volume * 65535) / 100;
		MIXERCONTROLDETAILS mixerControlDetails;
		mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
		mixerControlDetails.dwControlID = mixerControl.dwControlID;
		mixerControlDetails.cChannels = 1; //always 1 for a MIXERCONTROL_CONTROLF_UNIFORM control
		mixerControlDetails.cMultipleItems = 0; //always 0 except for a MIXERCONTROL_CONTROLF_MULTIPLE control
		mixerControlDetails.paDetails = &value;
		mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
		if (mixerSetControlDetails((HMIXEROBJ)mixerHandle, &mixerControlDetails, MIXER_SETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
		{
			LOG(LogError) << "VolumeControl::setVolume() - Failed to set mixer volume!";
		}
	}
	else if (endpointVolume != nullptr)
	{
		//Windows Vista or above. use EndpointVolume API
		float floatVolume = 0.0f; //0-1
		if (volume > 0) {
			floatVolume = (float)volume / 100.0f;
		}
		if (endpointVolume->SetMasterVolumeLevelScalar(floatVolume, nullptr) != S_OK)
		{
			LOG(LogError) << "VolumeControl::setVolume() - Failed to set master volume!";
		}
	}
#endif
}
