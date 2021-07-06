#include "CECInput.h"

#ifdef HAVE_LIBCEC
#include "Log.h"
#include <libcec/cec.h>
#include <iostream> // bad bad cecloader
#include <libcec/cecloader.h>
#include <SDL_events.h>
#ifdef _RPI_
extern "C" {
#include <interface/vmcs_host/vc_cecservice.h>
#include <interface/vmcs_host/vc_tvservice.h>
#include <interface/vmcs_host/vchost.h>
}
#endif // _RPI_
#endif // HAVE_LIBCEC

// hack for cec support
extern int SDL_USER_CECBUTTONDOWN;
extern int SDL_USER_CECBUTTONUP;

CECInput* CECInput::sInstance = nullptr;

#ifdef HAVE_LIBCEC
static void onAlert(void* /*cbParam*/, const CEC::libcec_alert type, const CEC::libcec_parameter param)
{
	LOG(LogDebug) << "CECInput::onAlert type: " << CECInput::getAlertTypeString(type) << " parameter: " << (char*)(param.paramData);

} // onAlert

static void onCommand(void* /*cbParam*/, const CEC::cec_command* command)
{
	LOG(LogDebug) << "CECInput::onCommand opcode: " << CECInput::getOpCodeString(command->opcode);

} // onCommand

static void onKeyPress(void* /*cbParam*/, const CEC::cec_keypress* key)
{
	LOG(LogDebug) << "CECInput::onKeyPress keycode: " << CECInput::getKeyCodeString(key->keycode);

	SDL_Event event;
	event.type      = (key->duration > 0) ? SDL_USER_CECBUTTONUP : SDL_USER_CECBUTTONDOWN;
	event.user.code = key->keycode;
	SDL_PushEvent(&event);

} // onKeyPress

static void onLogMessage(void* /*cbParam*/, const CEC::cec_log_message* message)
{
	LOG(LogDebug) << "CECInput::onLogMessage message: " << message->message;

} // onLogMessage

#ifdef _RPI_
static void vchi_tv_and_cec_init()
{
	VCHI_INSTANCE_T vchi_instance;
	VCHI_CONNECTION_T* vchi_connection;
	vc_host_get_vchi_state(&vchi_instance, &vchi_connection);

	vc_vchi_tv_init(vchi_instance, &vchi_connection, 1);
	vc_vchi_cec_init(vchi_instance, &vchi_connection, 1);

} // vchi_tv_and_cec_init

static void vchi_tv_and_cec_deinit()
{
	vc_vchi_cec_stop();
	vc_vchi_tv_stop();

} // vchi_tv_and_cec_deinit
#endif // _RPI_
#endif // HAVE_LIBCEC

void CECInput::init()
{
	if(!sInstance)
		sInstance = new CECInput();

} // init

void CECInput::deinit()
{
	if(sInstance)
	{
		delete sInstance;
		sInstance = nullptr;
	}

} // deinit

CECInput::CECInput() : mlibCEC(nullptr)
{

#ifdef HAVE_LIBCEC
#ifdef _RPI_
	// restart vchi tv and cec in case we just came back from another app using cec (like Kodi)
	vchi_tv_and_cec_deinit();
	vchi_tv_and_cec_init();
#endif // _RPI_

	CEC::ICECCallbacks        callbacks;
	CEC::libcec_configuration config;
	callbacks.Clear();
	config.Clear();

	callbacks.alert           = &onAlert;
	callbacks.commandReceived = &onCommand;
	callbacks.keyPress        = &onKeyPress;
	callbacks.logMessage      = &onLogMessage;

	sprintf(config.strDeviceName, "RetroPie ES");
	config.clientVersion   = CEC::LIBCEC_VERSION_CURRENT;
	config.bActivateSource = 0;
	config.callbacks       = &callbacks;
	config.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_PLAYBACK_DEVICE);

	mlibCEC = LibCecInitialise(&config);

	if(!mlibCEC)
	{
		LOG(LogError) << "CECInput::LibCecInitialise failed";
		return;
	}

	CEC::cec_adapter_descriptor adapters[10];
	int                         numAdapters = mlibCEC->DetectAdapters(adapters, 10, nullptr, true);

	if(numAdapters <= 0)
	{
		LOG(LogError) << "CECInput::mAdapter->DetectAdapters failed";
		UnloadLibCec(mlibCEC);
		mlibCEC = nullptr;
		return;
	}

	for(int i = 0; i < numAdapters; ++i)
		LOG(LogDebug) << "CEC adapter: " << i << " path: " << adapters[i].strComPath << " name: " << adapters[i].strComName;

	if(!mlibCEC->Open(adapters[0].strComName))
	{
		LOG(LogError) << "CECInput::mAdapter->Open failed";
		UnloadLibCec(mlibCEC);
		mlibCEC = nullptr;
		return;
	}
#endif // HAVE_LIBCEC

} // CECInput

CECInput::~CECInput()
{

#ifdef HAVE_LIBCEC
	if(mlibCEC)
	{
		mlibCEC->Close();
		UnloadLibCec(mlibCEC);
		mlibCEC = nullptr;
	}

#ifdef _RPI_
	// deinit vchi tv and cec in case we are going to launch another app using cec (like Kodi)
	vchi_tv_and_cec_deinit();
#endif // _RPI_
#endif // HAVE_LIBCEC

} // ~CECInput

std::string CECInput::getAlertTypeString(const unsigned int _type)
{
	switch(_type)
	{

#ifdef HAVE_LIBCEC
		case CEC::CEC_ALERT_SERVICE_DEVICE:         { return "Service-Device";         } break;
		case CEC::CEC_ALERT_CONNECTION_LOST:        { return "Connection-Lost";        } break;
		case CEC::CEC_ALERT_PERMISSION_ERROR:       { return "Permission-Error";       } break;
		case CEC::CEC_ALERT_PORT_BUSY:              { return "Port-Busy";              } break;
		case CEC::CEC_ALERT_PHYSICAL_ADDRESS_ERROR: { return "Physical-Address-Error"; } break;
		case CEC::CEC_ALERT_TV_POLL_FAILED:         { return "TV-Poll-Failed";         } break;
#else // HAVE_LIBCEC
		case 0:
#endif // HAVE_LIBCEC

		default:                                    { return "Unknown";                } break;
	}

} // getAlertTypeString

std::string CECInput::getOpCodeString(const unsigned int _opCode)
{
	switch(_opCode)
	{

#ifdef HAVE_LIBCEC
		case CEC::CEC_OPCODE_ACTIVE_SOURCE:                 { return "Active-Source";                 } break;
		case CEC::CEC_OPCODE_IMAGE_VIEW_ON:                 { return "Image-View-On";                 } break;
		case CEC::CEC_OPCODE_TEXT_VIEW_ON:                  { return "Text-View-On";                  } break;
		case CEC::CEC_OPCODE_INACTIVE_SOURCE:               { return "Inactive-Source";               } break;
		case CEC::CEC_OPCODE_REQUEST_ACTIVE_SOURCE:         { return "Request-Active-Source";         } break;
		case CEC::CEC_OPCODE_ROUTING_CHANGE:                { return "Routing-Change";                } break;
		case CEC::CEC_OPCODE_ROUTING_INFORMATION:           { return "Routing-Information";           } break;
		case CEC::CEC_OPCODE_SET_STREAM_PATH:               { return "Set-Stream-Path";               } break;
		case CEC::CEC_OPCODE_STANDBY:                       { return "Standby";                       } break;
		case CEC::CEC_OPCODE_RECORD_OFF:                    { return "Record-Off";                    } break;
		case CEC::CEC_OPCODE_RECORD_ON:                     { return "Record-On";                     } break;
		case CEC::CEC_OPCODE_RECORD_STATUS:                 { return "Record-Status";                 } break;
		case CEC::CEC_OPCODE_RECORD_TV_SCREEN:              { return "Record-TV-Screen";              } break;
		case CEC::CEC_OPCODE_CLEAR_ANALOGUE_TIMER:          { return "Clear-Analogue-Timer";          } break;
		case CEC::CEC_OPCODE_CLEAR_DIGITAL_TIMER:           { return "Clear-Digital-Timer";           } break;
		case CEC::CEC_OPCODE_CLEAR_EXTERNAL_TIMER:          { return "Clear-External-Timer";          } break;
		case CEC::CEC_OPCODE_SET_ANALOGUE_TIMER:            { return "Set-Analogue-Timer";            } break;
		case CEC::CEC_OPCODE_SET_DIGITAL_TIMER:             { return "Set-Digital-Timer";             } break;
		case CEC::CEC_OPCODE_SET_EXTERNAL_TIMER:            { return "Set-External-Timer";            } break;
		case CEC::CEC_OPCODE_SET_TIMER_PROGRAM_TITLE:       { return "Set-Timer-Program-Title";       } break;
		case CEC::CEC_OPCODE_TIMER_CLEARED_STATUS:          { return "Timer-Cleared-Status";          } break;
		case CEC::CEC_OPCODE_TIMER_STATUS:                  { return "Timer-Status";                  } break;
		case CEC::CEC_OPCODE_CEC_VERSION:                   { return "CEC-Version";                   } break;
		case CEC::CEC_OPCODE_GET_CEC_VERSION:               { return "Get-CEC-Version";               } break;
		case CEC::CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:         { return "Give-Physical-Address";         } break;
		case CEC::CEC_OPCODE_GET_MENU_LANGUAGE:             { return "Get-Menu-Language";             } break;
		case CEC::CEC_OPCODE_REPORT_PHYSICAL_ADDRESS:       { return "Report-Physical-Address";       } break;
		case CEC::CEC_OPCODE_SET_MENU_LANGUAGE:             { return "Set-Menu-Language";             } break;
		case CEC::CEC_OPCODE_DECK_CONTROL:                  { return "Deck-Control";                  } break;
		case CEC::CEC_OPCODE_DECK_STATUS:                   { return "Deck-Status";                   } break;
		case CEC::CEC_OPCODE_GIVE_DECK_STATUS:              { return "Give-Deck-Status";              } break;
		case CEC::CEC_OPCODE_PLAY:                          { return "Play";                          } break;
		case CEC::CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS:      { return "Give-Tuner-Device-Status";      } break;
		case CEC::CEC_OPCODE_SELECT_ANALOGUE_SERVICE:       { return "Select-Analogue-Service";       } break;
		case CEC::CEC_OPCODE_SELECT_DIGITAL_SERVICE:        { return "Select-Digital-Service";        } break;
		case CEC::CEC_OPCODE_TUNER_DEVICE_STATUS:           { return "Tuner-Device-Status";           } break;
		case CEC::CEC_OPCODE_TUNER_STEP_DECREMENT:          { return "Tuner-Step-Decrement";          } break;
		case CEC::CEC_OPCODE_TUNER_STEP_INCREMENT:          { return "Tuner-Step-Increment";          } break;
		case CEC::CEC_OPCODE_DEVICE_VENDOR_ID:              { return "Device-Vendor-ID";              } break;
		case CEC::CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:         { return "Give-Device-Vendor-ID";         } break;
		case CEC::CEC_OPCODE_VENDOR_COMMAND:                { return "Vendor-Command";                } break;
		case CEC::CEC_OPCODE_VENDOR_COMMAND_WITH_ID:        { return "Vendor-Command-With-ID";        } break;
		case CEC::CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN:     { return "Vendor-Remote-Button-Down";     } break;
		case CEC::CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP:       { return "Vendor-Remote-Button-Up";       } break;
		case CEC::CEC_OPCODE_SET_OSD_STRING:                { return "Set-OSD-String";                } break;
		case CEC::CEC_OPCODE_GIVE_OSD_NAME:                 { return "Give-OSD-Name";                 } break;
		case CEC::CEC_OPCODE_SET_OSD_NAME:                  { return "Set-OSD-Name";                  } break;
		case CEC::CEC_OPCODE_MENU_REQUEST:                  { return "Menu-Request";                  } break;
		case CEC::CEC_OPCODE_MENU_STATUS:                   { return "Menu-Status";                   } break;
		case CEC::CEC_OPCODE_USER_CONTROL_PRESSED:          { return "User-Control-Pressed";          } break;
		case CEC::CEC_OPCODE_USER_CONTROL_RELEASE:          { return "User-Control-Release";          } break;
		case CEC::CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:      { return "Give-Device-Power-Status";      } break;
		case CEC::CEC_OPCODE_REPORT_POWER_STATUS:           { return "Report-Power-Status";           } break;
		case CEC::CEC_OPCODE_FEATURE_ABORT:                 { return "Feature-Abort";                 } break;
		case CEC::CEC_OPCODE_ABORT:                         { return "Abort";                         } break;
		case CEC::CEC_OPCODE_GIVE_AUDIO_STATUS:             { return "Give-Audio-Status";             } break;
		case CEC::CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS: { return "Give-System-Audio-Mode-Status"; } break;
		case CEC::CEC_OPCODE_REPORT_AUDIO_STATUS:           { return "Report-Audio-Status";           } break;
		case CEC::CEC_OPCODE_SET_SYSTEM_AUDIO_MODE:         { return "Set-System-Audio-Mode";         } break;
		case CEC::CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST:     { return "System-Audio-Mode-Request";     } break;
		case CEC::CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS:      { return "System-Audio-Mode-Status";      } break;
		case CEC::CEC_OPCODE_SET_AUDIO_RATE:                { return "Set-Audio-Rate";                } break;
		case CEC::CEC_OPCODE_START_ARC:                     { return "Start-Arc";                     } break;
		case CEC::CEC_OPCODE_REPORT_ARC_STARTED:            { return "Report-Arc-Started";            } break;
		case CEC::CEC_OPCODE_REPORT_ARC_ENDED:              { return "Report-Arc-Ended";              } break;
		case CEC::CEC_OPCODE_REQUEST_ARC_START:             { return "Request-Arc-Start";             } break;
		case CEC::CEC_OPCODE_REQUEST_ARC_END:               { return "Request-Arc-End";               } break;
		case CEC::CEC_OPCODE_END_ARC:                       { return "End-Arc";                       } break;
		case CEC::CEC_OPCODE_CDC:                           { return "CDC";                           } break;
		case CEC::CEC_OPCODE_NONE:                          { return "None";                          } break;
#else // HAVE_LIBCEC
		case 0:
#endif // HAVE_LIBCEC

		default:                                            { return "Unknown";                       } break;
	}

} // getOpCodeString

std::string CECInput::getKeyCodeString(const unsigned int _keyCode)
{
	switch(_keyCode)
	{

#ifdef HAVE_LIBCEC
		case CEC::CEC_USER_CONTROL_CODE_SELECT:                      { return "Select";                      } break;
		case CEC::CEC_USER_CONTROL_CODE_UP:                          { return "Up";                          } break;
		case CEC::CEC_USER_CONTROL_CODE_DOWN:                        { return "Down";                        } break;
		case CEC::CEC_USER_CONTROL_CODE_LEFT:                        { return "Left";                        } break;
		case CEC::CEC_USER_CONTROL_CODE_RIGHT:                       { return "Right";                       } break;
		case CEC::CEC_USER_CONTROL_CODE_RIGHT_UP:                    { return "Right-Up";                    } break;
		case CEC::CEC_USER_CONTROL_CODE_RIGHT_DOWN:                  { return "Right-Down";                  } break;
		case CEC::CEC_USER_CONTROL_CODE_LEFT_UP:                     { return "Left-Up";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_LEFT_DOWN:                   { return "Left-Down";                   } break;
		case CEC::CEC_USER_CONTROL_CODE_ROOT_MENU:                   { return "Root-Menu";                   } break;
		case CEC::CEC_USER_CONTROL_CODE_SETUP_MENU:                  { return "Setup-Menu";                  } break;
		case CEC::CEC_USER_CONTROL_CODE_CONTENTS_MENU:               { return "Contents-Menu";               } break;
		case CEC::CEC_USER_CONTROL_CODE_FAVORITE_MENU:               { return "Favorite-Menu";               } break;
		case CEC::CEC_USER_CONTROL_CODE_EXIT:                        { return "Exit";                        } break;
		case CEC::CEC_USER_CONTROL_CODE_TOP_MENU:                    { return "Top-Menu";                    } break;
		case CEC::CEC_USER_CONTROL_CODE_DVD_MENU:                    { return "DVD-Menu";                    } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER_ENTRY_MODE:           { return "Number-Entry-Mode";           } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER11:                    { return "Number11";                    } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER12:                    { return "Number12";                    } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER0:                     { return "Number0";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER1:                     { return "Number1";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER2:                     { return "Number2";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER3:                     { return "Number3";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER4:                     { return "Number4";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER5:                     { return "Number5";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER6:                     { return "Number6";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER7:                     { return "Number7";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER8:                     { return "Number8";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_NUMBER9:                     { return "Number9";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_DOT:                         { return "Dot";                         } break;
		case CEC::CEC_USER_CONTROL_CODE_ENTER:                       { return "Enter";                       } break;
		case CEC::CEC_USER_CONTROL_CODE_CLEAR:                       { return "Clear";                       } break;
		case CEC::CEC_USER_CONTROL_CODE_NEXT_FAVORITE:               { return "Next-Favorite";               } break;
		case CEC::CEC_USER_CONTROL_CODE_CHANNEL_UP:                  { return "Channel-Up";                  } break;
		case CEC::CEC_USER_CONTROL_CODE_CHANNEL_DOWN:                { return "Channel-Down";                } break;
		case CEC::CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL:            { return "Previous-Channel";            } break;
		case CEC::CEC_USER_CONTROL_CODE_SOUND_SELECT:                { return "Sound-Select";                } break;
		case CEC::CEC_USER_CONTROL_CODE_INPUT_SELECT:                { return "Input-Select";                } break;
		case CEC::CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION:         { return "Display-Information";         } break;
		case CEC::CEC_USER_CONTROL_CODE_HELP:                        { return "Help";                        } break;
		case CEC::CEC_USER_CONTROL_CODE_PAGE_UP:                     { return "Page-Up";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_PAGE_DOWN:                   { return "Page-Down";                   } break;
		case CEC::CEC_USER_CONTROL_CODE_POWER:                       { return "Power";                       } break;
		case CEC::CEC_USER_CONTROL_CODE_VOLUME_UP:                   { return "Volume-Up";                   } break;
		case CEC::CEC_USER_CONTROL_CODE_VOLUME_DOWN:                 { return "Volume-Down";                 } break;
		case CEC::CEC_USER_CONTROL_CODE_MUTE:                        { return "Mute";                        } break;
		case CEC::CEC_USER_CONTROL_CODE_PLAY:                        { return "Play";                        } break;
		case CEC::CEC_USER_CONTROL_CODE_STOP:                        { return "Stop";                        } break;
		case CEC::CEC_USER_CONTROL_CODE_PAUSE:                       { return "Pause";                       } break;
		case CEC::CEC_USER_CONTROL_CODE_RECORD:                      { return "Record";                      } break;
		case CEC::CEC_USER_CONTROL_CODE_REWIND:                      { return "Rewind";                      } break;
		case CEC::CEC_USER_CONTROL_CODE_FAST_FORWARD:                { return "Fast-Forward";                } break;
		case CEC::CEC_USER_CONTROL_CODE_EJECT:                       { return "Eject";                       } break;
		case CEC::CEC_USER_CONTROL_CODE_FORWARD:                     { return "Forward";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_BACKWARD:                    { return "Backward";                    } break;
		case CEC::CEC_USER_CONTROL_CODE_STOP_RECORD:                 { return "Stop-Record";                 } break;
		case CEC::CEC_USER_CONTROL_CODE_PAUSE_RECORD:                { return "Pause-Record";                } break;
		case CEC::CEC_USER_CONTROL_CODE_ANGLE:                       { return "Angle";                       } break;
		case CEC::CEC_USER_CONTROL_CODE_SUB_PICTURE:                 { return "Sub-Picture";                 } break;
		case CEC::CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND:             { return "Video-On-Demand";             } break;
		case CEC::CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE:    { return "Electronic-Program-Guide";    } break;
		case CEC::CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING:           { return "Timer-Programming";           } break;
		case CEC::CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION:       { return "Initial-Configuration";       } break;
		case CEC::CEC_USER_CONTROL_CODE_SELECT_BROADCAST_TYPE:       { return "Select-Broadcast-Type";       } break;
		case CEC::CEC_USER_CONTROL_CODE_SELECT_SOUND_PRESENTATION:   { return "Select-Sound-Presentation";   } break;
		case CEC::CEC_USER_CONTROL_CODE_PLAY_FUNCTION:               { return "Play-Function";               } break;
		case CEC::CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION:         { return "Pause-Play-Function";         } break;
		case CEC::CEC_USER_CONTROL_CODE_RECORD_FUNCTION:             { return "Record-Function";             } break;
		case CEC::CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION:       { return "Pause-Record-Function";       } break;
		case CEC::CEC_USER_CONTROL_CODE_STOP_FUNCTION:               { return "Stop-Function";               } break;
		case CEC::CEC_USER_CONTROL_CODE_MUTE_FUNCTION:               { return "Mute-Function";               } break;
		case CEC::CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION:     { return "Restore-Volume-Function";     } break;
		case CEC::CEC_USER_CONTROL_CODE_TUNE_FUNCTION:               { return "Tune-Function";               } break;
		case CEC::CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION:       { return "Select-Media-Function";       } break;
		case CEC::CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION:    { return "Select-AV-Input-Function";    } break;
		case CEC::CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION: { return "Select-Audio-Input-Function"; } break;
		case CEC::CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION:       { return "Power-Toggle-Function";       } break;
		case CEC::CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION:          { return "Power-Off-Function";          } break;
		case CEC::CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION:           { return "Power-On-Function";           } break;
		case CEC::CEC_USER_CONTROL_CODE_F1_BLUE:                     { return "F1-Blue";                     } break;
		case CEC::CEC_USER_CONTROL_CODE_F2_RED:                      { return "F2-Red";                      } break;
		case CEC::CEC_USER_CONTROL_CODE_F3_GREEN:                    { return "F3-Green";                    } break;
		case CEC::CEC_USER_CONTROL_CODE_F4_YELLOW:                   { return "F4-Yellow";                   } break;
		case CEC::CEC_USER_CONTROL_CODE_F5:                          { return "F5";                          } break;
		case CEC::CEC_USER_CONTROL_CODE_DATA:                        { return "Data";                        } break;
		case CEC::CEC_USER_CONTROL_CODE_AN_RETURN:                   { return "AN-Return";                   } break;
		case CEC::CEC_USER_CONTROL_CODE_AN_CHANNELS_LIST:            { return "AN-Channels-List";            } break;
#else // HAVE_LIBCEC
		case 0:
#endif // HAVE_LIBCEC

		default:                                                     { return "Unknown";                     } break;
	}

} // getKeyCodeString
