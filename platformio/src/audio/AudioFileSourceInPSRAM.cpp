#include "AudioFileSourceInPSRAM.h"

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

AudioFileSourceInPSRAM::AudioFileSourceInPSRAM()
	: buffer(nullptr),
	  buffer_size(0),
	  buffer_capacity(0),
	  read_pos(0),
	  opened(false) {}

bool AudioFileSourceInPSRAM::init(size_t max_size)
{
	Serial.printf("PSRAM total before: %u free: %u\n", ESP.getPsramSize(), ESP.getFreePsram());

	if (max_size == 0)
		return false;
	if (buffer)
	{
		heap_caps_free(buffer);
		buffer = nullptr;
	}
	buffer_capacity = max_size + 10; // add a bit of slack
	buffer = (uint8_t *)heap_caps_malloc(buffer_capacity, MALLOC_CAP_SPIRAM);
	if (!buffer)
	{
		Serial.printf("PSRAM alloc failed for %u bytes\n", (unsigned)buffer_capacity);
		buffer_capacity = 0;
		return false;
	}
	memset(buffer, 0, 16); // only touch a few bytes to test validity

	Serial.printf("PSRAM total after: %u free: %u\n", ESP.getPsramSize(), ESP.getFreePsram());

	return true;
}

AudioFileSourceInPSRAM::~AudioFileSourceInPSRAM()
{
	if (buffer)
	{
		heap_caps_free(buffer);
		buffer = nullptr;
	}
}

// ---------------------------------------------------------------------------
// Load a file into PSRAM buffer
// ---------------------------------------------------------------------------

bool AudioFileSourceInPSRAM::loadFromFile(File &file)
{
	if (!buffer || !file)
		return false;

	uint32_t start_ms = millis(); // ⏱ start timing

	size_t bytes_to_read = file.size();
	if (bytes_to_read > buffer_capacity)
		bytes_to_read = buffer_capacity;

	file.seek(0);
	buffer_size = file.read(buffer, bytes_to_read) - 10; // leave a bit of slack
	read_pos = 0;
	opened = (buffer_size > 0);

	uint32_t elapsed_ms = millis() - start_ms;
	float seconds = elapsed_ms / 1000.0f;
	float mb = buffer_size / 1048576.0f;
	float rate = (seconds > 0) ? (mb / seconds) : 0;

	Serial.printf("\nAudioFileSourceInPSRAM: loaded %.2f MB in %.2f s (%.2f MB/s)\n", mb, seconds, rate);

	return opened;
}

/*
bool AudioFileSourceInPSRAM::loadFromFile(File &file)
{
	if (!buffer || !file)
		return false;

	uint32_t start_ms = millis(); // ⏱ start timing

	size_t total_size = file.size();
	if (total_size > buffer_capacity)
		total_size = buffer_capacity;

	file.seek(0);
	buffer_size = 0;

	const size_t SRAM_CHUNK = 16384; // 16 KB
	static uint8_t *sram_buf = nullptr;

	// allocate once in internal SRAM (not PSRAM)
	if (!sram_buf)
		sram_buf = (uint8_t *)heap_caps_malloc(SRAM_CHUNK, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

	if (!sram_buf)
	{
		Serial.println("Failed to allocate internal SRAM buffer!");
		return false;
	}

	while (buffer_size < total_size)
	{
		size_t remaining = total_size - buffer_size;
		size_t to_read = (remaining > SRAM_CHUNK) ? SRAM_CHUNK : remaining;

		int32_t read_now = file.read(sram_buf, to_read); // SD -> SRAM
		if (read_now <= 0)
			break;

		memcpy(buffer + buffer_size, sram_buf, read_now); // SRAM -> PSRAM
		buffer_size += read_now;
	}

	if (buffer_size >= 10)
		buffer_size -= 10;

	read_pos = 0;
	opened = (buffer_size > 0);

	uint32_t elapsed_ms = millis() - start_ms;
	float seconds = elapsed_ms / 1000.0f;
	float mb = buffer_size / 1048576.0f;
	float rate = (seconds > 0) ? (mb / seconds) : 0;

	Serial.printf(
		"\nAudioFileSourceInPSRAM: loaded %.2f MB in %.2f s (%.2f MB/s)\n",
		mb, seconds, rate
	);

	return opened;
}


*/
// AudioFileSourceInPSRAM : loaded 3.58 MB in 9.49 s(0.38 MB / s)

// ---------------------------------------------------------------------------
// Clear/reset for re-use
// ---------------------------------------------------------------------------

void AudioFileSourceInPSRAM::clear()
{
	read_pos = 0;
	buffer_size = 0;
	opened = false;
}

// ---------------------------------------------------------------------------
// AudioFileSource interface
// ---------------------------------------------------------------------------

bool AudioFileSourceInPSRAM::open(const char *filename)
{
	(void)filename;
	read_pos = 0;
	opened = (buffer && buffer_size > 0);
	return opened;
}

bool AudioFileSourceInPSRAM::close()
{
	read_pos = 0;
	opened = false;
	return true;
}

bool AudioFileSourceInPSRAM::isOpen()
{
	return opened;
}

uint32_t AudioFileSourceInPSRAM::read(void *data, uint32_t len)
{
	if (!opened || !buffer || read_pos >= buffer_size)
		return 0;

	size_t bytes_left = buffer_size - read_pos;
	size_t to_read = (len < bytes_left) ? len : bytes_left;

	memcpy(data, buffer + read_pos, to_read);
	read_pos += to_read;
	return to_read;
}

bool AudioFileSourceInPSRAM::seek(int32_t pos, int dir)
{
	if (!opened)
		return false;

	size_t new_pos = 0;
	switch (dir)
	{
	case SEEK_SET:
		new_pos = pos;
		break;
	case SEEK_CUR:
		new_pos = read_pos + pos;
		break;
	case SEEK_END:
		new_pos = buffer_size + pos;
		break;
	default:
		new_pos = read_pos;
		break;
	}

	if (new_pos > buffer_size)
		new_pos = buffer_size;

	read_pos = new_pos;
	return true;
}

uint32_t AudioFileSourceInPSRAM::getPos()
{
	return read_pos;
}

uint32_t AudioFileSourceInPSRAM::getSize()
{
	return buffer_size;
}