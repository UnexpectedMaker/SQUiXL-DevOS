#pragma once

#include <esp_heap_caps.h>
#include <utils/json.h>

// PSRAM-backed allocator
template <class T>
struct PsramAllocator
{
		typedef T value_type;

		PsramAllocator() = default;

		template <class U>
		constexpr PsramAllocator(const PsramAllocator<U> &) noexcept {}

		[[nodiscard]] T *allocate(std::size_t n)
		{
			void *ptr = heap_caps_malloc(n * sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
			if (!ptr)
				throw std::bad_alloc();
			return static_cast<T *>(ptr);
		}

		void deallocate(T *p, std::size_t) noexcept
		{
			heap_caps_free(p);
		}
};

template <class T, class U>
bool operator==(const PsramAllocator<T> &, const PsramAllocator<U> &) { return true; }

template <class T, class U>
bool operator!=(const PsramAllocator<T> &, const PsramAllocator<U> &) { return false; }

// PSRAM-compatible JSON type
using psram_json = nlohmann::basic_json<
	std::map,
	std::vector,
	std::string,
	bool,
	std::int64_t,
	std::uint64_t,
	double,
	PsramAllocator,
	nlohmann::adl_serializer>;

// Optional: PSRAM-backed string type if needed directly
using psram_string = std::basic_string<char, std::char_traits<char>, PsramAllocator<char>>;