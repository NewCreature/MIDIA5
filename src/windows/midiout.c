#include <windows.h>
#include <mmsystem.h>
#include "midia5.h"

typedef struct
{

    HMIDIOUT output_device;
	MIDIOUTCAPS output_device_caps;

} MIDIA5_PLATFORM_DATA;

void * _midia5_init_output_platform_data(MIDIA5_OUTPUT_HANDLE * hp, int device)
{
    MIDIA5_PLATFORM_DATA * cm_data;
	int err;

    cm_data = malloc(sizeof(MIDIA5_PLATFORM_DATA));
    if(cm_data)
    {
        if(midiOutOpen(&cm_data->output_device, MIDI_MAPPER, 0, 0, CALLBACK_NULL) == MMSYSERR_NOERROR)
    	{
    		midiOutReset(cm_data->output_device);
    		return cm_data;
    	}
        free(cm_data);
    }
    return NULL;
}

static void send_data(HMIDIOUT midi_out, int midi_data)
{
	int message_length[8] = {3, 3, 3, 3, 2, 2, 3, 0};
	static int midi_message;
	static int midi_message_length;
	static int midi_message_pos;

	if(midi_data >= 0x80)
	{
		midi_message_length = message_length[(midi_data >> 4) & 0x07];
		midi_message = 0;
		midi_message_pos = 0;
	}
	if(midi_message_length > 0)
	{
		midi_message |= ((unsigned long)midi_data) << (midi_message_pos * 8);
		midi_message_pos++;
		if(midi_message_pos == midi_message_length)
		{
			midiOutShortMsg(midi_out, midi_message);
		}
	}
}

void _midia5_free_output_platform_data(MIDIA5_OUTPUT_HANDLE * hp)
{
    MIDIA5_PLATFORM_DATA * cm_data = (MIDIA5_PLATFORM_DATA *)hp->platform_data;

    midiOutClose(cm_data->output_device);
    free(cm_data);
}

void _midia5_platform_send_data(MIDIA5_OUTPUT_HANDLE * hp, int data)
{
    MIDIA5_PLATFORM_DATA * cm_data = (MIDIA5_PLATFORM_DATA *)hp->platform_data;

    send_data(cm_data->output_device, data);
}

void _midia5_platform_reset_output_device(MIDIA5_OUTPUT_HANDLE * hp)
{
    MIDIA5_PLATFORM_DATA * cm_data = (MIDIA5_PLATFORM_DATA *)hp->platform_data;

    midiOutReset(cm_data->output_device);
}

bool _midia5_platform_set_output_gain(MIDIA5_OUTPUT_HANDLE * hp, float gain)
{
    MIDIA5_PLATFORM_DATA * cm_data = (MIDIA5_PLATFORM_DATA *)hp->platform_data;

	return false;
}
