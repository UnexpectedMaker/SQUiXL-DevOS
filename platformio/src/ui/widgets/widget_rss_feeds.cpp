#include "ui/widgets/widget_rss_feeds.h"
#include "ui/ui_screen.h"
#include <algorithm>
#include <cstring>

const bool ENABLE_DEBUG = false; // Toggle debug logging

#define DEBUG_PRINT(...)               \
	do                                 \
	{                                  \
		if (ENABLE_DEBUG)              \
			Serial.print(__VA_ARGS__); \
	}                                  \
	while (0)
#define DEBUG_PRINTLN(...)               \
	do                                   \
	{                                    \
		if (ENABLE_DEBUG)                \
			Serial.println(__VA_ARGS__); \
	}                                    \
	while (0)
#define DEBUG_PRINTF(...)               \
	do                                  \
	{                                   \
		if (ENABLE_DEBUG)               \
			Serial.printf(__VA_ARGS__); \
	}                                   \
	while (0)

void widgetRSSFeeds::reset_refresh_timer()
{
	DEBUG_PRINTLN("DEBUG: reset_refresh_timer called");
	process_next_article = true;
	next_refresh = 0;
}

void widgetRSSFeeds::show_next_article()
{
	DEBUG_PRINTLN("DEBUG: show_next_article called");
	DEBUG_PRINTF("DEBUG: stored_articles size: %d\n", stored_articles.size());
	if (stored_articles.size() == 1)
	{
		DEBUG_PRINTLN("DEBUG: Only one article left, checking for more articles");
		if (!is_getting_more_articles)
		{
			is_getting_more_articles = true;
			DEBUG_PRINTF("DEBUG: is_getting_more_articles set to true, feed_url: %s\n", feed_url.c_str());
			if (settings.has_wifi_creds() && !feed_url.empty())
			{
				DEBUG_PRINTLN("DEBUG: WiFi credentials available, adding feed to queue");
				wifi_controller.add_to_queue(feed_url.c_str(), [this](bool success, const String &response) {
					DEBUG_PRINTLN("DEBUG: wifi_controller callback triggered");
					this->process_article_data(success, response);
				});
			}
			else
			{
				DEBUG_PRINTF("DEBUG: Cannot fetch feed. WiFi creds: %d, feed_url empty: %d\n", settings.has_wifi_creds(), feed_url.empty());
			}
		}
		else
		{
			DEBUG_PRINTLN("DEBUG: Already getting more articles, skipping fetch");
		}
	}

	if (stored_articles.size() > 0)
	{
		DEBUG_PRINTLN("DEBUG: Processing lines for display");
		process_lines();
		DEBUG_PRINTLN("DEBUG: Initiating fade animation");
		fade(0.0, 1.0, 500, false, true, nullptr);
	}
	else
	{
		DEBUG_PRINTLN("DEBUG: No articles to display");
	}

	is_busy = false;
	next_refresh = millis() + 1000;
	DEBUG_PRINTF("DEBUG: next_refresh set to %lu\n", next_refresh);
}

void widgetRSSFeeds::process_article_data(bool success, const String &response)
{
	DEBUG_PRINTLN("DEBUG: process_article_data called");
	DEBUG_PRINTF("DEBUG: Success: %d, Response length: %d\n", success, response.length());
	bool ok = true;
	stored_articles.clear();
	DEBUG_PRINTLN("DEBUG: Cleared stored_articles");

	if (!success || response.isEmpty())
	{
		DEBUG_PRINTLN("DEBUG: Fetch failed or response empty");
		next_update = 0;
		is_getting_more_articles = false;
		is_dirty = false;
		DEBUG_PRINTLN("DEBUG: Set next_update to 0, is_getting_more_articles to false, is_dirty to false");
		return;
	}

	enum class ParseState
	{
		NONE,
		ITEM,
		TITLE,
		PUBDATE,
		CREATOR,
		CATEGORY,
		DESCRIPTION
	};
	ParseState state = ParseState::NONE;
	std::string buffer;
	std::string content_buffer;
	ARTICLE temp_article("", "", "", "", "No");
	int article_count = 0;
	unsigned long current_millis = millis();
	static unsigned long boot_time_seconds = 0;

	if (boot_time_seconds == 0)
	{
		DEBUG_PRINTLN("DEBUG: Initializing boot_time_seconds");
		// Reference date: May 31, 2025, 12:00:00 UTC
		boot_time_seconds = parse_date("2025-05-31T12:00:00+00:00") - current_millis / 1000;
		DEBUG_PRINTF("DEBUG: boot_time_seconds set to %lu\n", boot_time_seconds);
	}

	DEBUG_PRINTLN("DEBUG: Starting XML parsing");
	size_t pos = 0;
	std::string response_str = response.c_str();
	DEBUG_PRINTF("DEBUG: Response string length: %zu\n", response_str.length());
	while (article_count < 15 && pos < response_str.length())
	{
		size_t tag_start = response_str.find('<', pos);
		if (tag_start == std::string::npos)
		{
			DEBUG_PRINTLN("DEBUG: No more '<' found, breaking parse loop");
			break;
		}
		size_t tag_end = response_str.find('>', tag_start);
		if (tag_end == std::string::npos)
		{
			DEBUG_PRINTLN("DEBUG: No closing '>' found, breaking parse loop");
			break;
		}
		std::string tag = response_str.substr(tag_start + 1, tag_end - tag_start - 1);
		pos = tag_end + 1;

		size_t space_pos = tag.find(' ');
		if (space_pos != std::string::npos)
		{
			tag = tag.substr(0, space_pos);
		}
		DEBUG_PRINTF("DEBUG: Found tag: %s at position %zu\n", tag.c_str(), tag_start);

		if (tag == "item" || tag == "entry")
		{
			state = ParseState::ITEM;
			temp_article = ARTICLE("", "", "", "", "No");
			buffer.clear();
			content_buffer.clear();
			DEBUG_PRINTLN("DEBUG: Entered new item/entry");
		}
		else if (state == ParseState::ITEM && tag[0] != '/')
		{
			if (tag == "title")
			{
				state = ParseState::TITLE;
				buffer.clear();
				content_buffer.clear();
				DEBUG_PRINTLN("DEBUG: Parsing title");
			}
			else if (tag == "pubDate" || tag == "published" || tag == "dc:date")
			{
				state = ParseState::PUBDATE;
				buffer.clear();
				content_buffer.clear();
				DEBUG_PRINTLN("DEBUG: Parsing pubDate/published/dc:date");
			}
			else if (tag == "author" || tag == "dc:creator")
			{
				state = ParseState::CREATOR;
				buffer.clear();
				content_buffer.clear();
				DEBUG_PRINTLN("DEBUG: Parsing author/dc:creator");
			}
			else if (tag == "category" || tag == "slash:section")
			{
				state = ParseState::CATEGORY;
				buffer.clear();
				content_buffer.clear();
				DEBUG_PRINTLN("DEBUG: Parsing category/slash:section");
			}
			else if (tag == "description" || tag == "slash:comments")
			{
				state = ParseState::DESCRIPTION;
				buffer.clear();
				content_buffer.clear();
				DEBUG_PRINTLN("DEBUG: Parsing description/slash:comments");
			}

			if (state != ParseState::ITEM && state != ParseState::NONE)
			{
				size_t content_end = response_str.find('<', pos);
				if (content_end == std::string::npos)
					content_end = response_str.length();
				buffer = response_str.substr(pos, content_end - pos);
				if (buffer.find("<![CDATA[") == 0)
				{
					buffer = buffer.substr(9);
					size_t cdata_end = buffer.rfind("]]>");
					if (cdata_end != std::string::npos)
					{
						buffer = buffer.substr(0, cdata_end);
					}
				}
				size_t first_non_space = buffer.find_first_not_of(" \t\r\n");
				size_t last_non_space = buffer.find_last_not_of(" \t\r\n");
				if (first_non_space != std::string::npos && last_non_space != std::string::npos)
				{
					buffer = buffer.substr(first_non_space, last_non_space - first_non_space + 1);
				}
				else
				{
					buffer.clear();
				}
				content_buffer = buffer;
				DEBUG_PRINTF("DEBUG: Collected content_buffer: %s\n", content_buffer.c_str());
				pos = content_end;
			}
		}

		if (tag[0] == '/')
		{
			std::string closing_tag = tag.substr(1);
			DEBUG_PRINTF("DEBUG: Closing tag: %s\n", closing_tag.c_str());
			if (closing_tag == "item" || closing_tag == "entry")
			{
				DEBUG_PRINTF("DEBUG: Checking article: headline=%s\n", temp_article.headline.c_str());
				if (!temp_article.headline.empty())
				{
					stored_articles.push_back(temp_article);
					article_count++;
					DEBUG_PRINTF("DEBUG: Added article #%d: headline=%s, date=%s, creator=%s, subject=%s, comments=%s\n", article_count, temp_article.headline.c_str(), temp_article.date.c_str(), temp_article.creator.c_str(), temp_article.subject.c_str(), temp_article.comments.c_str());
				}
				else
				{
					DEBUG_PRINTLN("DEBUG: Skipped article due to empty headline");
				}
				state = ParseState::NONE;
				buffer.clear();
				content_buffer.clear();
			}
			else if (state == ParseState::TITLE && closing_tag == "title")
			{
				temp_article.headline = content_buffer;
				DEBUG_PRINTF("DEBUG: Set headline: %s\n", temp_article.headline.c_str());
				state = ParseState::ITEM;
				buffer.clear();
				content_buffer.clear();
			}
			else if (state == ParseState::PUBDATE && (closing_tag == "pubDate" || closing_tag == "published" || closing_tag == "dc:date"))
			{
				unsigned long article_timestamp = parse_date(content_buffer);
				unsigned long current_time_seconds = boot_time_seconds + current_millis / 1000;
				unsigned long seconds_ago = (article_timestamp > 0 && current_time_seconds > article_timestamp) ? (current_time_seconds - article_timestamp) : 0;
				temp_article.date = format_time_ago(seconds_ago);
				DEBUG_PRINTF("DEBUG: Set date: %s (timestamp=%lu, seconds_ago=%lu)\n", temp_article.date.c_str(), article_timestamp, seconds_ago);
				state = ParseState::ITEM;
				buffer.clear();
				content_buffer.clear();
			}
			else if (state == ParseState::CREATOR && (closing_tag == "author" || closing_tag == "dc:creator"))
			{
				temp_article.creator = content_buffer;
				DEBUG_PRINTF("DEBUG: Set creator: %s\n", temp_article.creator.c_str());
				state = ParseState::ITEM;
				buffer.clear();
				content_buffer.clear();
			}
			else if (state == ParseState::CATEGORY && (closing_tag == "category" || closing_tag == "slash:section"))
			{
				temp_article.subject = content_buffer;
				DEBUG_PRINTF("DEBUG: Set subject: %s\n", temp_article.subject.c_str());
				state = ParseState::ITEM;
				buffer.clear();
				content_buffer.clear();
			}
			else if (state == ParseState::DESCRIPTION && (closing_tag == "description" || closing_tag == "slash:comments"))
			{
				temp_article.comments = content_buffer.empty() ? "No" : content_buffer;
				DEBUG_PRINTF("DEBUG: Set comments: %s\n", temp_article.comments.c_str());
				state = ParseState::ITEM;
				buffer.clear();
				content_buffer.clear();
			}
		}
	}

	DEBUG_PRINTF("DEBUG: Parsing complete, added %d articles\n", stored_articles.size());

	if (ok)
	{
		if (!is_getting_more_articles && stored_articles.size() > 0)
		{
			DEBUG_PRINTLN("DEBUG: Resetting refresh timer due to successful parse");
			reset_refresh_timer();
		}
		is_getting_more_articles = false;
		DEBUG_PRINTLN("DEBUG: Set is_getting_more_articles to false");
	}

	is_dirty = ok;
	DEBUG_PRINTF("DEBUG: Set is_dirty to %d\n", is_dirty);
}

bool widgetRSSFeeds::redraw(uint8_t fade_amount, int8_t tab_group)
{
	if (millis() < delay_first_draw)
		return false;

	DEBUG_PRINTLN("DEBUG: redraw called");
	DEBUG_PRINTF("DEBUG: fade_amount=%d, tab_group=%d\n", fade_amount, tab_group);
	bool was_dirty = false;

	if (process_next_article)
	{
		DEBUG_PRINTLN("DEBUG: process_next_article is true, calling show_next_article");
		process_next_article = false;
		show_next_article();
		return false;
	}

	if (!is_setup)
	{
		DEBUG_PRINTLN("DEBUG: Setting up widget");
		is_setup = true;

		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);
		DEBUG_PRINTF("DEBUG: char_width=%d, char_height=%d\n", char_width, char_height);

		max_chars_per_line = (_w - 20) / char_width;
		max_lines = (_h - 60) / char_height;
		DEBUG_PRINTF("DEBUG: max_chars_per_line=%d, max_lines=%d\n", max_chars_per_line, max_lines);

		_sprite_article.createVirtual(_w, _h, NULL, true);
		DEBUG_PRINTF("DEBUG: Created virtual sprite, width=%d, height=%d\n", _w, _h);

		feed_url = settings.config.rss_feed.feed_url.c_str();
		DEBUG_PRINTF("DEBUG: Set feed_url: %s\n", feed_url.c_str());

		if (settings.has_wifi_creds() && !feed_url.empty() && !wifi_controller.wifi_blocking_access)
		{
			DEBUG_PRINTLN("DEBUG: WiFi credentials available, adding feed to queue");
			wifi_controller.add_to_queue(feed_url.c_str(), [this](bool success, const String &response) {
				DEBUG_PRINTLN("DEBUG: wifi_controller callback triggered in setup");
				this->process_article_data(success, response);
			});
		}
		else
		{
			DEBUG_PRINTF("DEBUG: Cannot fetch feed in setup. WiFi creds: %d, feed_url empty: %d, wifi_blocking_access: %d\n", settings.has_wifi_creds(), feed_url.empty(), wifi_controller.wifi_blocking_access);
		}
	}

	if (is_busy)
	{
		DEBUG_PRINTLN("DEBUG: Widget is busy, returning false");
		return false;
	}

	is_busy = true;
	DEBUG_PRINTLN("DEBUG: Set is_busy to true");

	if (is_dirty_hard)
	{
		DEBUG_PRINTLN("DEBUG: is_dirty_hard is true, updating background");
		ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
		ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		delay(10);

		draw_window_heading();
		DEBUG_PRINTLN("DEBUG: Drew window heading");

		_sprite_article.fillScreen(TFT_MAGENTA);
		DEBUG_PRINTLN("DEBUG: Filled sprite with magenta");

		is_dirty_hard = false;
		is_aniamted_cached = false;
		was_dirty = true;
		DEBUG_PRINTLN("DEBUG: Set is_dirty_hard to false, is_aniamted_cached to false, was_dirty to true");
	}

	if (!is_aniamted_cached)
	{
		DEBUG_PRINTF("DEBUG: is_aniamted_cached is false, lines size: %zu\n", lines.size());
		if (lines.size() > 0)
		{
			int16_t start_y = 42;
			_sprite_article.setFreeFont(UbuntuMono_R[1]);
			_sprite_article.setTextColor(TFT_CYAN, -1);
			DEBUG_PRINTLN("DEBUG: Set font and text color for drawing lines");

			for (size_t l = 0; l < std::min(lines.size(), (size_t)max_lines); l++)
			{
				if (lines[l] == "*nl*")
				{
					start_y += 5;
					_sprite_article.setTextColor(TFT_YELLOW, -1);
					DEBUG_PRINTLN("DEBUG: Encountered *nl*, changed text color to yellow");
				}
				else
				{
					_sprite_article.setCursor(10, start_y);
					_sprite_article.print(lines[l].c_str());
					DEBUG_PRINTF("DEBUG: Drew line %zu: %s at y=%d\n", l, lines[l].c_str(), start_y);
					start_y += 20;
				}
			}

			was_dirty = true;
			is_aniamted_cached = true;
			DEBUG_PRINTLN("DEBUG: Set was_dirty to true, is_aniamted_cached to true");
		}
		else if (!settings.config.rss_feed.enabled)
		{
			_sprite_article.setFreeFont(UbuntuMono_R[2]);
			_sprite_article.setTextColor(TFT_RED - 1);
			_sprite_article.setCursor(10, 38);
			_sprite_article.print("NOT ENABLED!");
			DEBUG_PRINTLN("DEBUG: Displayed 'NOT ENABLED!'");
		}
		else if (!settings.config.rss_feed.has_url())
		{
			_sprite_article.setFreeFont(UbuntuMono_R[2]);
			_sprite_article.setTextColor(TFT_RED - 1);
			_sprite_article.setCursor(10, 38);
			_sprite_article.print("NO RSS FEED URL");
			DEBUG_PRINTLN("DEBUG: Displayed 'NO RSS FEED URL'");
		}
		else
		{
			_sprite_article.setFreeFont(UbuntuMono_R[2]);
			_sprite_article.setTextColor(TFT_GREY, -1);
			_sprite_article.setCursor(10, 38);
			_sprite_article.print("WAITING...");
			DEBUG_PRINTLN("DEBUG: Displayed 'WAITING...'");
		}
	}

	if (fade_amount < 32)
	{
		squixl.lcd.blendSprite(&_sprite_article, &_sprite_back, &_sprite_mixed, fade_amount, TFT_MAGENTA);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
		DEBUG_PRINTF("DEBUG: Blended sprite with fade_amount=%d\n", fade_amount);
	}
	else
	{
		squixl.lcd.blendSprite(&_sprite_article, &_sprite_back, &_sprite_mixed, 32, TFT_MAGENTA);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
		DEBUG_PRINTLN("DEBUG: Blended sprite with max fade_amount");
	}

	if (is_dirty && !was_dirty)
		was_dirty = true;
	DEBUG_PRINTF("DEBUG: is_dirty=%d, was_dirty=%d\n", is_dirty, was_dirty);

	is_dirty = false;
	is_busy = false;
	DEBUG_PRINTLN("DEBUG: Set is_dirty to false, is_busy to false");

	next_refresh = millis();
	DEBUG_PRINTF("DEBUG: next_refresh set to %lu\n", next_refresh);

	return (fade_amount < 32 || was_dirty);
}

bool widgetRSSFeeds::process_touch(touch_event_t touch_event)
{
	DEBUG_PRINTLN("DEBUG: process_touch called");
	DEBUG_PRINTF("DEBUG: touch_event type: %d, x: %d, y: %d\n", touch_event.type, touch_event.x, touch_event.y);
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			DEBUG_PRINTLN("DEBUG: Touch within bounds");
			if (is_busy)
			{
				DEBUG_PRINTLN("DEBUG: Widget is busy, ignoring touch");
				return false;
			}

			if (millis() - next_click_update > 1000)
			{
				next_click_update = millis();
				DEBUG_PRINTF("DEBUG: next_click_update set to %lu\n", next_click_update);
				if (stored_articles.size() > 1)
				{
					DEBUG_PRINTLN("DEBUG: Removing first article");
					audio.play_tone(505, 12);
					stored_articles.erase(stored_articles.begin());
				}

				next_refresh = millis() + 1000;
				DEBUG_PRINTF("DEBUG: next_refresh set to %lu\n", next_refresh);
				DEBUG_PRINTLN("DEBUG: Initiating fade animation for touch");
				fade(1.0, 0.0, 500, false, false, [this]() { reset_refresh_timer(); });
				return true;
			}
			else
			{
				DEBUG_PRINTLN("DEBUG: Touch ignored due to click update cooldown");
			}
		}
		else
		{
			DEBUG_PRINTLN("DEBUG: Touch outside bounds");
		}
	}

	return false;
}

void widgetRSSFeeds::process_lines()
{
	DEBUG_PRINTLN("DEBUG: process_lines called");
	lines.clear();
	DEBUG_PRINTLN("DEBUG: Cleared lines vector");
	squixl.split_text_into_lines(String(stored_articles[0].headline.c_str()), max_chars_per_line, lines);
	DEBUG_PRINTF("DEBUG: Split headline: %s into %zu lines\n", stored_articles[0].headline.c_str(), lines.size());

	lines.push_back("*nl*");
	DEBUG_PRINTLN("DEBUG: Added *nl* separator");

	std::string metadata;
	metadata.reserve(128);
	metadata += stored_articles[0].date + " | " + stored_articles[0].creator + " | " +
				stored_articles[0].subject + " | " + stored_articles[0].comments + " comments.";
	DEBUG_PRINTF("DEBUG: Constructed metadata: %s\n", metadata.c_str());

	squixl.split_text_into_lines(String(metadata.c_str()), max_chars_per_line, lines);
	DEBUG_PRINTF("DEBUG: Split metadata into %zu lines (total lines: %zu)\n", lines.size() - 1, lines.size());

	_sprite_article.fillScreen(TFT_MAGENTA);
	DEBUG_PRINTLN("DEBUG: Filled sprite with magenta");
	is_aniamted_cached = false;
	DEBUG_PRINTLN("DEBUG: Set is_aniamted_cached to false");
}

unsigned long widgetRSSFeeds::parse_date(const std::string &date_str)
{
	DEBUG_PRINTF("DEBUG: parse_date called with: %s\n", date_str.c_str());
	if (date_str.empty())
	{
		DEBUG_PRINTLN("DEBUG: Date string empty, returning 0");
		return 0;
	}

	int year = 1970, month = 1, day = 1, hour = 0, minute = 0, second = 0;
	bool valid = false;

	// Handle ISO 8601 (e.g., "2025-05-31T12:00:00+00:00")
	if (date_str.length() >= 19 && date_str[4] == '-' && date_str[7] == '-' && date_str[10] == 'T')
	{
		year = std::atoi(date_str.substr(0, 4).c_str());
		month = std::atoi(date_str.substr(5, 2).c_str());
		day = std::atoi(date_str.substr(8, 2).c_str());
		hour = std::atoi(date_str.substr(11, 2).c_str());
		minute = std::atoi(date_str.substr(14, 2).c_str());
		second = std::atoi(date_str.substr(17, 2).c_str());
		valid = (year >= 1970 && month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour <= 23 && minute <= 59 && second <= 59);
		DEBUG_PRINTF("DEBUG: Parsed ISO 8601 date: %d-%d-%d %d:%d:%d, valid=%d\n", year, month, day, hour, minute, second, valid);
	}
	// Handle RFC 822 (e.g., "Wed, 31 May 2025 12:00:00 GMT")
	else
	{
		size_t pos = date_str.find(',');
		if (pos != std::string::npos)
		{
			std::string rest = date_str.substr(pos + 2);
			size_t space1 = rest.find(' ');
			size_t space2 = rest.find(' ', space1 + 1);
			size_t space3 = rest.find(' ', space2 + 1);
			size_t space4 = rest.find(' ', space3 + 1);
			if (space1 != std::string::npos && space2 != std::string::npos &&
				space3 != std::string::npos && space4 != std::string::npos)
			{
				day = std::atoi(rest.substr(0, space1).c_str());
				std::string month_str = rest.substr(space1 + 1, space2 - space1 - 1);
				year = std::atoi(rest.substr(space2 + 1, space3 - space2 - 1).c_str());
				std::string time = rest.substr(space3 + 1, space4 - space3 - 1);
				hour = std::atoi(time.substr(0, 2).c_str());
				minute = std::atoi(time.substr(3, 2).c_str());
				second = std::atoi(time.substr(6, 2).c_str());

				static const std::string months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
				for (int i = 0; i < 12; ++i)
				{
					if (month_str == months[i])
					{
						month = i + 1;
						break;
					}
				}
				valid = (year >= 1970 && month >= 1 && month <= 12 && day >= 1 && day <= 31 && hour <= 23 && minute <= 59 && second <= 59);
				DEBUG_PRINTF("DEBUG: Parsed RFC 822 date: %d-%d-%d %d:%d:%d, month_str=%s, valid=%d\n", year, month, day, hour, minute, second, month_str.c_str(), valid);
			}
			else
			{
				DEBUG_PRINTLN("DEBUG: RFC 822 date format invalid");
			}
		}
		else
		{
			DEBUG_PRINTLN("DEBUG: No comma found in date string");
		}
	}

	if (!valid)
	{
		DEBUG_PRINTLN("DEBUG: Invalid date, returning 0");
		return 0;
	}

	// Calculate days since epoch (1970-01-01)
	unsigned long days = 0;
	for (int y = 1970; y < year; ++y)
	{
		days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
	}
	static const int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	for (int m = 1; m < month; ++m)
	{
		days += days_in_month[m - 1];
		if (m == 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
		{
			days += 1;
		}
	}
	days += day - 1;
	unsigned long seconds = days * 86400 + hour * 3600 + minute * 60 + second;
	DEBUG_PRINTF("DEBUG: Calculated seconds since epoch: %lu\n", seconds);
	return seconds;
}

std::string widgetRSSFeeds::format_time_ago(unsigned long seconds_ago)
{
	DEBUG_PRINTF("DEBUG: format_time_ago called with seconds_ago=%lu\n", seconds_ago);
	if (seconds_ago < 10)
	{
		return "just now";
	}
	else if (seconds_ago < 60)
	{
		return std::to_string(seconds_ago) + " seconds ago";
	}
	else if (seconds_ago < 3600)
	{
		unsigned long minutes = seconds_ago / 60;
		return std::to_string(minutes) + (minutes == 1 ? " minute ago" : " minutes ago");
	}
	else if (seconds_ago < 86400)
	{
		unsigned long hours = seconds_ago / 3600;
		return std::to_string(hours) + (hours == 1 ? " hour ago" : " hours ago");
	}
	else if (seconds_ago < 604800)
	{
		unsigned long days = seconds_ago / 86400;
		return std::to_string(days) + (days == 1 ? " day ago" : " days ago");
	}
	else
	{
		unsigned long weeks = seconds_ago / 604800;
		return std::to_string(weeks) + (weeks == 1 ? " week ago" : " weeks ago");
	}
}

widgetRSSFeeds widget_rss_feeds;