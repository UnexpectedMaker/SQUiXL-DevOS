#ifndef _AUDIOFILESOURCEINPSRAM_H_
#define _AUDIOFILESOURCEINPSRAM_H_

#include <Arduino.h>
#include <FS.h>
#include <esp_heap_caps.h>
#include "AudioFileSource.h"

class AudioFileSourceInPSRAM : public AudioFileSource
{
	public:
		AudioFileSourceInPSRAM(); // default, no allocation
		virtual ~AudioFileSourceInPSRAM();

		// call this when you know the max size
		bool init(size_t max_size);

		bool loadFromFile(File &file);
		void clear();

		bool open(const char *filename) override;
		bool close() override;
		bool isOpen() override;
		uint32_t read(void *data, uint32_t len) override;
		bool seek(int32_t pos, int dir) override;
		uint32_t getPos() override;
		uint32_t getSize() override;

		uint8_t *getBuffer() const { return buffer; }
		size_t getCapacity() const { return buffer_capacity; }

	private:
		uint8_t *buffer;
		size_t buffer_size;
		size_t buffer_capacity;
		size_t read_pos;
		bool opened;
};

#endif