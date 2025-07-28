#pragma once

// #define INT_RADIO

#include <map>
#include <queue>
#include <string>
#include <vector>
#include "Arduino.h"
#include "AudioFileSourcePROGMEM.h"
#include "AudioFileSourceFunction.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#ifdef INT_RADIO
#include "AudioFileSourceICYStream.h"
#include "AudioFileSourceBuffer.h"
#include "AudioGeneratorMP3.h"
#include "AudioGeneratorAAC.h"
#endif
#include "AudioOutputMixer.h"

#include "audio/voice/voice_goodbye.h"
#include "audio/voice/voice_hello.h"
#include "audio/voice/voice_squixl.h"
#include "audio/voice/voice_unexpectedmaker.h"

#include "audio/sfx/sfx_button.h"
#include "utils/json_psram.h"

// #include "settings/settings_async.h"

struct SFX
{
		const int8_t *array;
		uint16_t size;

		SFX() : array(nullptr), size(0) {}
		SFX(const int8_t *_array, uint16_t _size) : array(_array), size(_size)
		{
			// Serial.printf("Added SFX with %u bytes\n", size);
		}
};

class AudioClass
{

	public:
		static float hz;

		float max_volume = 40.0;

		void setup(int8_t pin_data, int8_t pin_bclk, int8_t _pin_lrclk, int8_t _pin_sd_mode);
		void stop();

		void play_dock();

		void play_wav(const char *wav_name, bool force = false);
		void play_wav_queue(const char *wav_name);
		void play_wav_char_squence(const String &word);
		void play_ip_address();
		void play_tone(float _hz, float _tm);
		void play_note(int index, int y, float vol_fade = 1.0, float duration = 20.0);
		float get_piano_key_freq(uint8_t key);
		void play_menu_beep(int index, int y);
		void update();
		void set_volume(uint8_t vol);
		static float sine_wave(const float time);

		inline bool queue_is_empty()
		{
			return queue.empty();
		}

		void wait_for_finish();

	private:
		bool state = false;

		float current_volume = 15.0;

		// I2S Audio Out
		AudioOutputI2S *out = nullptr;

		// Local playback
		AudioGeneratorWAV *wav = nullptr;
		AudioFileSourcePROGMEM *file = nullptr;
		AudioFileSourceFunction *func = nullptr;

#ifdef INT_RADIO
// // Internet radio
// AudioGeneratorAAC *acc = nullptr;
// AudioGeneratorMP3 *mp3 = nullptr;
// AudioFileSourceBuffer *buff = nullptr;
// AudioFileSourceICYStream *icys_stream = nullptr;
#endif

		// Audio Mixer
		AudioOutputMixer *mixer = nullptr;
		AudioOutputMixerStub *mixer_channels[2] = {nullptr, nullptr};

		const int preallocateBufferSize = 16 * 1024;
		const int preallocateCodecSize = 85332; // AAC+SBR codec max mem needed

		void *preallocateBuffer = NULL;
		void *preallocateCodec = NULL;

		bool is_icys_stream_connected = false;
		unsigned long streaming_music_update = 0;

		std::vector<psram_string> queue;

		std::map<psram_string, SFX> wav_files;

		std::vector<const char *> stations;

		std::vector<float> piano_notes = {
			1046.50,
			1108.73,
			1174.66,
			1244.51,
			1318.51,
			1396.91,
			1479.98,
			1567.98,
			1661.22,
			1760.00,
			1864.66,
			1975.53
		};
};

extern AudioClass audio;