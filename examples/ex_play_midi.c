#include <allegro5/allegro5.h>
#include "midia5.h"
#include "rtk/midi.h"
#include "rtk/io_allegro.h"

typedef struct
{

	MIDIA5_OUTPUT_HANDLE * output_handle;
	RTK_MIDI * midi;
	bool paused;
	double elapsed_time;
	double end_time;
	ALLEGRO_MUTEX * mutex;
	ALLEGRO_THREAD * thread;
	unsigned long current_tick;
	double event_time;
	int midi_event[32];
	float volume;

} CODEC_DATA;

static void * codec_load_file(const char * fn, const char * subfn)
{
	CODEC_DATA * data;
	int i;

	data = malloc(sizeof(CODEC_DATA));
	if(data)
	{
		memset(data, 0, sizeof(CODEC_DATA));
		data->midi = rtk_load_midi(fn);
		if(!data->midi)
		{
			free(data);
			return NULL;
		}
		data->mutex = al_create_mutex();
		if(!data->mutex)
		{
			rtk_destroy_midi(data->midi);
			free(data);
			return NULL;
		}
		data->end_time = 0.0;
		for(i = 0; i < data->midi->tracks; i++)
		{
			if(data->midi->track[i]->event[data->midi->track[i]->events - 1]->pos_sec > data->end_time)
			{
				data->end_time = data->midi->track[i]->event[data->midi->track[i]->events - 1]->pos_sec;
			}
		}
		data->volume = 1.0;
	}
	return data;
}

static void codec_unload_file(void * data)
{
	CODEC_DATA * codec_data = (CODEC_DATA *)data;

	al_destroy_mutex(codec_data->mutex);
	rtk_destroy_midi(codec_data->midi);
	free(data);
}

static bool codec_set_volume(void * data, float volume)
{
	CODEC_DATA * codec_data = (CODEC_DATA *)data;

	al_lock_mutex(codec_data->mutex);
	midia5_set_output_gain(codec_data->output_handle, volume);
	al_unlock_mutex(codec_data->mutex);
	return true;
}

static unsigned long get_next_event_tick(RTK_MIDI * mp, unsigned long current_tick, int * track_event, double * event_time)
{
	int i, j;
	unsigned long track_tick[32] = {0};
	double track_time[32] = {0.0};
	unsigned long largest_tick = 0;
	unsigned long shortest_tick;
	double shortest_time = 10000.0;

	for(i = 0; i < mp->tracks; i++)
	{
		for(j = track_event[i]; j < mp->track[i]->events; j++)
		{
			if(mp->track[i]->event[j]->tick > current_tick)
			{
				track_tick[i] = mp->track[i]->event[j]->tick;
				track_time[i] = mp->track[i]->event[j]->pos_sec;
				if(track_tick[i] > largest_tick)
				{
					largest_tick = track_tick[i];
				}
				break;
			}
		}
	}
	shortest_tick = largest_tick;
	for(i = 0; i < mp->tracks; i++)
	{
		if(track_event[i] < mp->track[i]->events && track_tick[i] <= shortest_tick)
		{
			shortest_tick = track_tick[i];
			shortest_time = track_time[i];
		}
	}
	*event_time = shortest_time;
	return shortest_tick;
}

static void send_midi_event_data(MIDIA5_OUTPUT_HANDLE * hp, RTK_MIDI * mp, unsigned long tick, int * track_event, float volume)
{
	int i;

	for(i = 0; i < mp->tracks; i++)
	{
		while(track_event[i] < mp->track[i]->events && mp->track[i]->event[track_event[i]]->tick == tick)
		{
			switch(mp->track[i]->event[track_event[i]]->type)
			{
				case RTK_MIDI_EVENT_TYPE_NOTE_OFF:
				case RTK_MIDI_EVENT_TYPE_NOTE_ON:
				case RTK_MIDI_EVENT_TYPE_KEY_AFTER_TOUCH:
				case RTK_MIDI_EVENT_TYPE_CONTROLLER_CHANGE:
				case RTK_MIDI_EVENT_TYPE_PITCH_WHEEL_CHANGE:
				{
					midia5_send_data(hp, mp->track[i]->event[track_event[i]]->type + mp->track[i]->event[track_event[i]]->channel);
					midia5_send_data(hp, mp->track[i]->event[track_event[i]]->data_i[0]);
					midia5_send_data(hp, (float)mp->track[i]->event[track_event[i]]->data_i[1] * volume);
					break;
				}
				case RTK_MIDI_EVENT_TYPE_PROGRAM_CHANGE:
				case RTK_MIDI_EVENT_TYPE_CHANNEL_AFTER_TOUCH:
				{
					midia5_send_data(hp, mp->track[i]->event[track_event[i]]->type + mp->track[i]->event[track_event[i]]->channel);
					midia5_send_data(hp, mp->track[i]->event[track_event[i]]->data_i[0]);
					break;
				}
			}
			track_event[i]++;
		}
	}
}

static void * codec_thread_proc(ALLEGRO_THREAD * thread, void * arg)
{
	CODEC_DATA * codec_data = (CODEC_DATA *)arg;
	ALLEGRO_EVENT_QUEUE * queue;
	ALLEGRO_TIMER * timer;
	ALLEGRO_EVENT event;
	double tick_time = 1.0 / 1000.0;

	queue = al_create_event_queue();
	if(!queue)
	{
		return NULL;
	}
	timer = al_create_timer(tick_time);
	if(!timer)
	{
		al_destroy_event_queue(queue);
		return NULL;
	}
	al_register_event_source(queue, al_get_timer_event_source(timer));
	al_start_timer(timer);
	send_midi_event_data(codec_data->output_handle, codec_data->midi, codec_data->current_tick, codec_data->midi_event, codec_data->volume);
	codec_data->current_tick = get_next_event_tick(codec_data->midi, codec_data->current_tick, codec_data->midi_event, &codec_data->event_time);
	while(!al_get_thread_should_stop(thread) && codec_data->elapsed_time < codec_data->end_time)
	{
		al_wait_for_event(queue, &event);
		if(!codec_data->paused)
		{
			al_lock_mutex(codec_data->mutex);
			codec_data->elapsed_time += tick_time;
			if(codec_data->elapsed_time >= codec_data->event_time)
			{
				send_midi_event_data(codec_data->output_handle, codec_data->midi, codec_data->current_tick, codec_data->midi_event, codec_data->volume);
				codec_data->current_tick = get_next_event_tick(codec_data->midi, codec_data->current_tick, codec_data->midi_event, &codec_data->event_time);
			}
			al_unlock_mutex(codec_data->mutex);
		}
	}
	al_unregister_event_source(queue, al_get_timer_event_source(timer));
	al_destroy_timer(timer);
	al_destroy_event_queue(queue);
	return NULL;
}

static bool codec_play(void * data)
{
	CODEC_DATA * codec_data = (CODEC_DATA *)data;

	codec_data->output_handle = midia5_create_output_handle(0);
	if(codec_data->output_handle)
	{
		midia5_reset_output_device(codec_data->output_handle);
		codec_data->thread = al_create_thread(codec_thread_proc, codec_data);
		if(codec_data->thread)
		{
			al_start_thread(codec_data->thread);
		}
		return true;
	}
	return false;
}

int main(int argc, char * argv[])
{
	CODEC_DATA * codec_data;
	MIDIA5_OUTPUT_HANDLE * output_handle;
	ALLEGRO_TIMER * timer;
	ALLEGRO_EVENT_QUEUE * queue;
	ALLEGRO_EVENT event;
	bool done = false;

	if(argc < 2)
	{
		printf("Usage: ex_play_midi <midi file>\n");
		return -1;
	}
	rtk_io_set_allegro_driver();
	codec_data = codec_load_file(argv[1], NULL);
	if(!codec_data)
	{
		printf("Failed to load midi file: %s\n", argv[1]);
		return -1;
	}
	output_handle = midia5_create_output_handle(0);
	if(!output_handle)
	{
		printf("Unable to create output handle!\n");
		return -1;
	}
	if(!al_init())
	{
		printf("Unable to initialize Allegro!\n");
		return -1;
	}
	if(!al_install_keyboard())
	{
		printf("Unable to initialie keyboard!\n");
		return -1;
	}
	timer = al_create_timer(1.0 / 60.0);
	if(!timer)
	{
		printf("Unable to create timer!\n");
		return -1;
	}
	queue = al_create_event_queue();
	if(!queue)
	{
		printf("Unable to create event queue!\n");
		return -1;
	}
	al_register_event_source(queue, al_get_timer_event_source(timer));
	al_register_event_source(queue, al_get_keyboard_event_source());
	al_start_timer(timer);
	codec_play(codec_data);
	while(!done)
	{
		al_wait_for_event(queue, &event);
		switch(event.type)
		{
			case ALLEGRO_EVENT_KEY_DOWN:
			{
				if(event.keyboard.keycode == ALLEGRO_KEY_ESCAPE)
				{
					done = true;
				}
				break;
			}
		}
	}
	return 0;
}
