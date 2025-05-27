#include "ui/widgets/widget_rss_feeds.h"
#include "ui/ui_screen.h"

void widgetRSSFeeds::reset_refresh_timer()
{
	process_next_article = true;
	next_refresh = 0;
}

void widgetRSSFeeds::show_next_article()
{
	if (stored_articles.size() == 1)
	{
		if (!is_getting_more_articles)
		{
			is_getting_more_articles = true;
			if (settings.has_wifi_creds() && !feed_url.empty())
			{
				wifi_controller.add_to_queue(feed_url, [this](bool success, const String &response) { this->process_article_data(success, response); });
			}
		}
	}

	if (stored_articles.size() > 0)
	{
		process_lines();
		fade(0.0, 1.0, 500, false, true, nullptr);
	}

	is_busy = false;
	next_refresh = millis() + 1000;
}

void widgetRSSFeeds::process_article_data(bool success, const String &response)
{
	bool ok = true;
	stored_articles.clear(); // Clear existing articles to refresh with new ones
	try
	{
		if (response == "ERROR")
		{

			next_update = 0;
			return;
		}

		// Simple XML parsing to extract the first 5 articles
		int article_count = 0;
		int pos = 0;
		unsigned long current_millis = millis();
		static unsigned long boot_time_seconds = 0;

		if (boot_time_seconds == 0)
		{
			// Initialize with an arbitrary recent timestamp (e.g., 2025-05-21T00:00:00)
			boot_time_seconds = parse_iso8601_date("2025-05-21T00:00:00+00:00") - current_millis / 1000;
		}

		while (article_count < 5 && pos < response.length())
		{
			// Find the next <item> tag
			int item_start = response.indexOf("<item rdf:about", pos);
			if (item_start == -1)
				break;
			int item_end = response.indexOf("</item>", item_start);
			if (item_end == -1)
				break;

			while (article_count < 5 && pos < response.length())
			{
				// Find the next <item> tag
				int item_start = response.indexOf("<item rdf:about", pos);
				if (item_start == -1)
					break;
				int item_end = response.indexOf("</item>", item_start);
				if (item_end == -1)
					break;

				String item_content = response.substring(item_start, item_end + 7);

				// Extract title
				String headline = "";
				int title_start = item_content.indexOf("<title>") + 7;
				int title_end = item_content.indexOf("</title>", title_start);
				if (title_start != -1 && title_end != -1)
				{
					headline = item_content.substring(title_start, title_end);
				}
				// Extract date and convert to timestamp
				String date = "";
				unsigned long article_timestamp = 0;
				int date_start = item_content.indexOf("<dc:date>") + 9;
				int date_end = item_content.indexOf("</dc:date>", date_start);
				if (date_start != -1 && date_end != -1)
				{
					date = item_content.substring(date_start, date_end);
					article_timestamp = parse_iso8601_date(date);
					// Compute time difference in seconds
					unsigned long current_time_seconds = boot_time_seconds + current_millis / 1000;
					unsigned long seconds_ago = (current_time_seconds > article_timestamp) ? (current_time_seconds - article_timestamp) : 0;
					date = format_time_ago(seconds_ago);
				}

				// Extract creator
				String creator = "";
				int creator_start = item_content.indexOf("<dc:creator>") + 12;
				int creator_end = item_content.indexOf("</dc:creator>", creator_start);
				if (creator_start != -1 && creator_end != -1)
				{
					creator = item_content.substring(creator_start, creator_end);
				}

				// Extract subject
				String subject = "";
				int subject_start = item_content.indexOf("<slash:section>") + 15;
				int subject_end = item_content.indexOf("</slash:section>", subject_start);
				if (subject_start != -1 && subject_end != -1)
				{
					subject = item_content.substring(subject_start, subject_end);
				}

				// Extract Comments
				String comments = "No";
				int comments_start = item_content.indexOf("<slash:comments>") + 16;
				int comments_end = item_content.indexOf("</slash:comments>", comments_start);
				if (comments_start != -1 && comments_end != -1)
				{
					comments = item_content.substring(comments_start, comments_end);
				}

				if (headline.length() > 0)
				{
					stored_articles.push_back(ARTICLE(headline, date, creator, subject, comments));
					article_count++;
				}

				pos = item_end + 7;
			}
		}
		Serial.printf("Added RSS Feed articles, you now have %d\n\n", stored_articles.size());
	}
	catch (std::exception &e)
	{
		// Serial.printf("response: %s\n", response.c_str());
		// Serial.println("XML parse error:");
		// Serial.println(e.what());
		next_update = millis();
		ok = false;
	}

	if (ok)
	{
		if (!is_getting_more_articles)
		{
			if (stored_articles.size() > 0)
			{
				reset_refresh_timer();
			}
		}
		is_getting_more_articles = false;
	}

	is_dirty = ok;
}

bool widgetRSSFeeds::redraw(uint8_t fade_amount, int8_t tab_group)
{
	bool was_dirty = false;

	if (process_next_article)
	{
		process_next_article = false;
		show_next_article();
		return false;
	}

	if (!is_setup)
	{
		is_setup = true;

		squixl.get_cached_char_sizes(FONT_SPEC::FONT_WEIGHT_R, 1, &char_width, &char_height);

		max_chars_per_line = int((_w - 20) / char_width); // includes padding for margins
		max_lines = int((_h - 60) / char_height);		  // includes padding for margins ahd top heading

		_sprite_article.createVirtual(_w, _h, NULL, true);

		feed_url = settings.config.rss_feed.feed_url.c_str();

		if (settings.has_wifi_creds() && !feed_url.empty() && !wifi_controller.wifi_blocking_access)
		{
			wifi_controller.add_to_queue(feed_url, [this](bool success, const String &response) { this->process_article_data(success, response); });
		}
	}

	if (is_busy)
	{
		return false;
	}

	is_busy = true;

	if (is_dirty_hard)
	{
		ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_clean.getBuffer());
		ui_parent->_sprite_back.readImage(_x, _y, _w, _h, (uint16_t *)_sprite_back.getBuffer());
		delay(10);

		draw_window_heading();

		_sprite_article.fillScreen(TFT_MAGENTA);

		is_dirty_hard = false;
		is_aniamted_cached = false;
		was_dirty = true;
	}

	if (!is_aniamted_cached)
	{
		if (lines.size() > 0)
		{
			int16_t start_y = 42;
			_sprite_article.setFreeFont(UbuntuMono_R[1]);
			_sprite_article.setTextColor(TFT_CYAN, -1);

			for (int l = 0; l < min((uint8_t)lines.size(), max_lines); l++)
			{
				if (lines[l] == "*nl*")
				{
					start_y += 5;
					_sprite_article.setTextColor(TFT_YELLOW, -1);
				}
				else
				{
					_sprite_article.setCursor(10, start_y);
					_sprite_article.print(lines[l]);
					start_y += 20;
				}
			}

			was_dirty = true;
			is_aniamted_cached = true;
		}
		else if (!settings.config.rss_feed.enabled)
		{
			_sprite_article.setTextColor(TFT_RED - 1);
			_sprite_article.setCursor(10, 38);
			_sprite_article.print("NOT ENABLED!");
			// Serial.println("Not Enabled");
		}
		else if (!settings.config.rss_feed.has_url())
		{
			_sprite_article.setTextColor(TFT_RED - 1);
			_sprite_article.setCursor(10, 38);
			_sprite_article.print("NO RSS FEED URL");
			// Serial.println("No RSS Feed URL");
		}
		else
		{
			_sprite_article.setTextColor(TFT_GREY, -1);
			_sprite_article.setCursor(10, 38);
			_sprite_article.print("WAITING...");
			// Serial.println("Waiting");
		}
	}

	if (fade_amount < 32)
	{
		squixl.lcd.blendSprite(&_sprite_article, &_sprite_back, &_sprite_mixed, fade_amount, TFT_MAGENTA);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}
	else
	{
		squixl.lcd.blendSprite(&_sprite_article, &_sprite_back, &_sprite_mixed, 32, TFT_MAGENTA);
		ui_parent->_sprite_content.drawSprite(_x, _y, &_sprite_mixed, 1.0f, -1, DRAW_TO_RAM);
	}

	if (is_dirty && !was_dirty)
		was_dirty = true;

	is_dirty = false;
	is_busy = false;

	next_refresh = millis();

	return (fade_amount < 32 || was_dirty);
}

bool widgetRSSFeeds::process_touch(touch_event_t touch_event)
{
	if (touch_event.type == TOUCH_TAP)
	{
		if (check_bounds(touch_event.x, touch_event.y))
		{
			if (is_busy)
				return false;

			if (millis() - next_click_update > 1000)
			{
				next_click_update = millis();
				if (stored_articles.size() > 1)
				{
					audio.play_tone(505, 12);
					stored_articles.erase(stored_articles.begin());
				}

				next_refresh = millis() + 1000;
				fade(1.0, 0.0, 500, false, false, [this]() { reset_refresh_timer(); });
				return true;
			}
		}
	}

	return false;
}

void widgetRSSFeeds::process_lines()
{
	// Serial.println("process_lines");
	lines.clear();
	squixl.split_text_into_lines(stored_articles[0].headline, max_chars_per_line, lines);

	lines.push_back("*nl*");

	String metadata;
	metadata.reserve(128); // Optional, if you expect the metadata to be long
	metadata += stored_articles[0].date;
	metadata += " | ";
	metadata += stored_articles[0].creator;
	metadata += " | ";
	metadata += stored_articles[0].subject;
	metadata += " | ";
	metadata += stored_articles[0].comments;
	metadata += " comments.";

	// String metadata = stored_articles[0].date + " | " + stored_articles[0].creator + " | " + stored_articles[0].subject + " | " + stored_articles[0].comments + " comments.";
	squixl.split_text_into_lines(metadata, max_chars_per_line, lines);

	_sprite_article.fillScreen(TFT_MAGENTA);
	is_aniamted_cached = false;
}

unsigned long widgetRSSFeeds::parse_iso8601_date(const String &date_str)
{
	// Example format: "2025-05-20T21:20:00+00:00"
	int year = date_str.substring(0, 4).toInt();
	int month = date_str.substring(5, 7).toInt();
	int day = date_str.substring(8, 10).toInt();
	int hour = date_str.substring(11, 13).toInt();
	int minute = date_str.substring(14, 16).toInt();
	int second = date_str.substring(17, 19).toInt();

	unsigned long days = (year - 1970) * 365 + (month - 1) * 30 + day;
	unsigned long seconds = days * 86400 + hour * 3600 + minute * 60 + second;
	return seconds;
}

// Helper function to format time difference
String widgetRSSFeeds::format_time_ago(unsigned long seconds_ago)
{
	if (seconds_ago < 60)
	{
		return String(seconds_ago) + " seconds ago";
	}
	else if (seconds_ago < 3600)
	{
		return String(seconds_ago / 60) + " minutes ago";
	}
	else if (seconds_ago < 86400)
	{
		return String(seconds_ago / 3600) + " hours ago";
	}
	else if (seconds_ago < 604800)
	{
		return String(seconds_ago / 86400) + " days ago";
	}
	else
	{
		return String(seconds_ago / 604800) + " weeks ago";
	}
}

widgetRSSFeeds widget_rss_feeds;
