#include "audio/audio.h"
#include "WiFi.h"
#include "web/wifi_controller.h"

float hhz = 440;
float tm = 1.0;
float vol = 0.5;

#ifdef INT_RADIO
// Called when a metadata event occurs (i.e. an ID3 tag, an ICY block, etc.
void MDCallback(void *cbData, const char *type, bool isUnicode, const char *string)
{
	const char *ptr = reinterpret_cast<const char *>(cbData);
	(void)isUnicode; // Punt this ball for now
	// Note that the type and string may be in PROGMEM, so copy them to RAM for printf
	char s1[32], s2[64];
	strncpy_P(s1, type, sizeof(s1));
	s1[sizeof(s1) - 1] = 0;
	strncpy_P(s2, string, sizeof(s2));
	s2[sizeof(s2) - 1] = 0;
	Serial.printf("METADATA(%s) '%s' = '%s'\n", ptr, s1, s2);
	Serial.flush();
}

int status_count = 0;
// Called when there's a warning or error (like a buffer underflow or decode hiccup)
void StatusCallback(void *cbData, int code, const char *string)
{
	if (code != 257)
	{
		// if (status_count < 10)
		// {
		// 	status_count++;

		const char *ptr = reinterpret_cast<const char *>(cbData);
		// Note that the string may be in PROGMEM, so copy it to RAM for printf
		char s1[64];
		strncpy_P(s1, string, sizeof(s1));
		s1[sizeof(s1) - 1] = 0;
		Serial.printf("STATUS(%s) '%d' = '%s'\n", ptr, code, s1);
		Serial.flush();
	}
}
#endif

// audio.setup(MUX_D3, MUX_D4, MUX_D2, MUX_D1);
void AudioClass::setup(int8_t pin_data, int8_t pin_bclk, int8_t _pin_lrclk, int8_t _pin_sd_mode)
{

	if (wav_files.size() == 0)
	{
		wav_files = {
			{"goodbye", SFX(voice_goodbye, sizeof(voice_goodbye))},
			{"hello", SFX(voice_hello, sizeof(voice_hello))},
			{"um", SFX(voice_um, sizeof(voice_um))},
			{"squixl", SFX(voice_squixl, sizeof(voice_squixl))},
		};

		if (false)
		{
			for (const auto &entry : wav_files)
			{
				// entry.first is the key (name)
				// entry.second is the SFX structure
				Serial.print("Name: ");
				Serial.println(entry.first.c_str());

				Serial.print("Size: ");
				Serial.println(entry.second.size);

				// Optionally, print a few bytes of the array data (if that's desired)
				Serial.print("Data (first 10 bytes): ");
				// Use a limit to prevent printing too much data
				int bytesToPrint = entry.second.size < 10 ? entry.second.size : 10;
				for (int i = 0; i < bytesToPrint; i++)
				{
					// Print in hexadecimal format
					Serial.print(entry.second.array[i], HEX);
					Serial.print(" ");
				}
				Serial.println("\n----------------");
			}
		}
	}

#ifdef INT_RADIO
	if (stations.size() == 0)
	{
		stations.push_back("http://playerservices.streamtheworld.com/api/livestream-redirect/TLPSTR18.mp3");
		stations.push_back("http://22323.live.streamtheworld.com:80/TLPSTR18.mp3");
		stations.push_back("http://listen.technobase.fm/tunein-mp3");

		stations.push_back("http://0n-80s.radionetz.de:8000/0n-70s.mp3");
		stations.push_back("http://www.partyviberadio.com:8010/listen.pls?sid=1");
		stations.push_back("https://www.danceattack.fm/");
		stations.push_back("https://icecast-bulteam.cdnvideo.ru/bolid128");
	}
#endif

	// We need to re-init the peripheral every time we are switching back.
	if (!state)
	{
		out = new AudioOutputI2S();
		wav = new AudioGeneratorWAV();

		mixer = new AudioOutputMixer(64, out);

		// Local playback channel
		mixer_channels[0] = mixer->NewInput();
		mixer_channels[0]->SetGain(1.0);

		audioLogger = &Serial;

#ifdef INT_RADIO
		// First, preallocate all the memory needed for the buffering and codecs, never to be freed
		// Using PSRAM as we have stack so fit, so making big buffers!!!!

		// This is only for web streaming - not needed right now!
		// preallocateBuffer = ps_malloc(preallocateBufferSize);
		// preallocateCodec = ps_malloc(preallocateCodecSize);
		// if (!preallocateBuffer || !preallocateCodec)
		// {
		// 	Serial.printf("\n\n***FATAL ERROR:  Unable to preallocate %d bytes for app\n\n", preallocateBufferSize + preallocateCodecSize);
		// }
#endif
		out->SetPinout(pin_bclk, _pin_lrclk, pin_data);
		// out->SetRate(22050); // 44100

		pinMode(_pin_sd_mode, OUTPUT);
		digitalWrite(_pin_sd_mode, HIGH);

		Serial.println("I2S Peripheral is now setup");

		state = true;
	}

	streaming_music_update = millis();
}

void AudioClass::stop()
{
	if (state)
	{
		if (out->stop())
		{
			state = false;
			Serial.println("I2S Peripheral is now stopped");

			is_icys_stream_connected = false;

			if (wav)
				wav->stop();

#ifdef INT_RADIO
			// if (mp3)
			// 	mp3->stop();

			// if (acc)
			// 	acc->stop();

			// if (buff)
			// 	buff->close();

			// if (icys_stream)
			// 	icys_stream->close();
#endif

			mixer_channels[0]->stop();
			// mixer_channels[1]->stop();

			delete out;
			delete wav;
			delete file;
			delete mixer;

#ifdef INT_RADIO
			// delete buff;
			// delete icys_stream;
			// delete mp3;
			// delete acc;
#endif
			out = nullptr;
			wav = nullptr;
			file = nullptr;
			mixer = nullptr;
#ifdef INT_RADIO
// buff = nullptr;
// icys_stream = nullptr;
// mp3 = nullptr;
// acc = nullptr;
#endif
		}
	}
}

float AudioClass::sine_wave(const float time)
{
	float v = cos(TWO_PI * hhz * time * tm);
	v *= vol;
	vol = constrain(vol - 0.0001, 0.00, 1.0); // Fade down over time
	return v;
}

void AudioClass::play_tone(float _hz, float _tm)
{
	if (out == nullptr || wav == nullptr)
		return;

	if (current_volume == 0)
		return;

	hhz = _hz + 660;
	tm = _tm;

	// Playing this tone via a function doesn't seem to respect the I2S Amps volume settings, so we compensate here
	// It seems 1/6 of the volume % seems about right
	vol = ((float)current_volume / max_volume) / 6.0;
	if (!wav->isRunning())
	{
		func = new AudioFileSourceFunction(20);
		func->addAudioGenerators([this](const float time) { return sine_wave(time); });
		wav->begin(func, mixer_channels[0]);
	}
}

void AudioClass::play_dock()
{
	if (out == nullptr || wav == nullptr)
		return;

	if (current_volume == 0)
		return;

	hhz = 1400;
	tm = 2;

	// Playing this loud
	vol = ((float)current_volume / max_volume) / 3.0;
	if (!wav->isRunning())
	{
		func = new AudioFileSourceFunction(20);
		func->addAudioGenerators([this](const float time) { return sine_wave(time); });
		wav->begin(func, mixer_channels[0]);
	}
}

void AudioClass::play_menu_beep(int index, int y)
{
	if (current_volume == 0)
		return;

	hhz = piano_notes[index];
	tm = 1.0;

	// Playing this tone via a function doesn't seem to respect the I2S Amps volume settings, so we compensate here
	// It seems 1/6 of the volume % seems about right
	vol = ((float)current_volume / max_volume) / 6.0;
	if (wav->isRunning())
		wav->stop();
	func = new AudioFileSourceFunction(0.25);
	func->addAudioGenerators([this](const float time) { return sine_wave(time); });
	wav->begin(func, mixer_channels[0]);
}

float AudioClass::get_piano_key_freq(uint8_t key)
{
	// A4 is the 49th key and has a frequency of 440 Hz
	int A4_keyNumber = 49;
	float A4_frequency = 440.0;

	// Calculate the frequency using the 12-tone equal temperament formula
	float frequency = pow(2.0, (key - A4_keyNumber) / 12.0) * A4_frequency;

	return frequency;
}

void AudioClass::play_note(int index, int y, float vol_fade, float duration)
{
	if (current_volume == 0)
		return;

	if (y < 6)
		index += 12;
	hhz = get_piano_key_freq(52 + index);
	tm = 1.0;

	// Playing this tone via a function doesn't seem to respect the I2S Amps volume settings, so we compensate here
	// It seems 1/6 of the volume % seems about right
	vol = ((float)current_volume / max_volume) / 6.0;
	vol *= vol_fade;
	if (!wav->isRunning())
	{
		func = new AudioFileSourceFunction(duration);
		func->addAudioGenerators([this](const float time) { return sine_wave(time); });
		wav->begin(func, mixer_channels[0]);
	}
}

void AudioClass::set_volume(uint8_t vol)
{
	current_volume = vol;
	out->SetGain((float)vol / max_volume);
}

void AudioClass::play_wav(const char *wav_name, bool force)
{
	if (current_volume == 0)
		return;

	if (file != nullptr)
		delete file;

	std::string key(wav_name);

	if (wav_files.find(key) != wav_files.end())
	{
		SFX &snd = wav_files[key];
		Serial.printf("Playing wav %s, length %d\n", key.c_str(), snd.size);
		file = new AudioFileSourcePROGMEM(snd.array, snd.size);

		wav->begin(file, mixer_channels[0]);
	}
	else
	{
		Serial.printf("error snd %s not found or invalid\n", wav_name);
	}
}

void AudioClass::play_wav_queue(const char *wav_name)
{
	if (current_volume == 0)
		return;

	if (wav == nullptr || out == nullptr)
		return;

	if (wav->isRunning())
	{
		Serial.printf("adding %s to queue\n", wav_name);
		queue.push_back(wav_name);
	}
	else
	{
		std::string key(wav_name);
		Serial.printf("loading wav %s to queue,\n", key.c_str());

		if (wav_files.find(key) != wav_files.end())
		{
			SFX &snd = wav_files[key];
			file = new AudioFileSourcePROGMEM(snd.array, snd.size);
			wav->begin(file, mixer_channels[0]);
		}
		else
		{
			Serial.println("error snd not found");
		}
	}
}

void AudioClass::play_wav_char_squence(const String &word)
{
	for (int i = 0; i < word.length(); i++)
	{
		String sound_name = "num_" + word.substring(i, i + 1);
		const char *sound_name_char = sound_name.c_str();

		Serial.printf("adding %s to queue\n", sound_name.c_str());
		queue.push_back(strdup(sound_name_char));
	}
}

void AudioClass::play_ip_address()
{
	if (current_volume == 0)
		return;

	IPAddress ip = WiFi.localIP();
	String ip_str = ip.toString();

	int i = 0;
	while (i < ip_str.length())
	{
		String sound_name = "num_" + ip_str.substring(i, i + 1);
		const char *sound_name_char = sound_name.c_str();

		if (wav->isRunning())
			continue;

		Serial.printf("playing %s\n", sound_name_char);

		if (wav_files.find(sound_name_char) != wav_files.end())
		{
			SFX &snd = wav_files[sound_name_char];
			file = new AudioFileSourcePROGMEM(snd.array, snd.size);
			wav->begin(file, mixer_channels[0]);
		}
		else
		{
			Serial.println("error snd not found");
		}
		i++;
	}
}

void AudioClass::update()
{

	// if (!is_icys_stream_connected && wifi_controller.is_connected())
	// {
	// 	if (millis() - streaming_music_update > 5000)
	// 	{
	// 		Serial.println("\nRADIO: Initialising the streaming stuff - maybe?");

	// 		// Internet radio channel
	// 		mixer_channels[1] = mixer->NewInput();
	// 		mixer_channels[1]->SetGain(0.5);

	// 		Serial.print("\nRADIO: Opening station: ");
	// 		Serial.println(stations[settings.config.audio.current_radio_station]);

	// 		icys_stream = new AudioFileSourceICYStream(stations[settings.config.audio.current_radio_station]);
	// 		icys_stream->RegisterMetadataCB(MDCallback, (void *)"ICY");
	// 		// icys_stream->RegisterMetadataCB(MDCallback, NULL);

	// 		buff = new AudioFileSourceBuffer(icys_stream, preallocateBuffer, preallocateBufferSize);
	// 		buff->RegisterStatusCB(StatusCallback, (void *)"buffer");
	// 		// buff->RegisterStatusCB(StatusCallback, NULL);

	// 		// acc = new AudioGeneratorAAC(preallocateCodec, preallocateCodecSize);
	// 		// acc->RegisterStatusCB(StatusCallback, (void *)"acc");
	// 		// acc->begin(buff, mixer_channels[1]);

	// 		mp3 = new AudioGeneratorMP3(preallocateCodec, preallocateCodecSize);
	// 		mp3->RegisterStatusCB(StatusCallback, (void *)"mp3");
	// 		mp3->begin(buff, mixer_channels[1]);

	// 		is_icys_stream_connected = true;
	// 	}
	// }

	if (out == nullptr || wav == nullptr)
		return;

	static int lastms = 0;

	// if (is_icys_stream_connected && millis() - streaming_music_update > 1000)
	// {
	// 	if (mp3->isRunning())
	// 	{
	// 		if (millis() - lastms > 1000)
	// 		{
	// 			lastms = millis();
	// 			Serial.printf("Running for %d ms...\n", lastms);
	// 			Serial.flush();
	// 		}
	// 		if (!mp3->loop())
	// 		{
	// 			mp3->stop();
	// 			// mixer_channels[1]->stop();
	// 		}
	// 	}
	// 	else
	// 	{
	// 		// Serial.printf("MP3 done\n");
	// 		streaming_music_update = millis();
	// 		// delay(1000);
	// 	}
	// }

	if (wav->isRunning())
	{
		if (!wav->loop())
		{
			wav->stop();
			// mixer_channels[0]->stop();
		}
	}
	else if (!queue.empty())
	{
		if (current_volume == 0)
			return;

		Serial.printf("playing %s from queue\n", queue.front().c_str());
		play_wav(queue.front().c_str());
		queue.erase(queue.begin());
	}
}

void AudioClass::wait_for_finish()
{
	while (audio.wav->isRunning())
	{
		if (!audio.wav->loop())
		{
			audio.wav->stop();
		}
		yield;
	}
}

AudioClass audio;