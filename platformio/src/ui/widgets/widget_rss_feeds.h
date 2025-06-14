#pragma once

#include "ui/ui_window.h"
#include <vector>
#include <string>

struct ARTICLE
{
		std::string headline;
		std::string date;
		std::string creator;
		std::string subject;
		std::string comments;

		ARTICLE(const std::string &h, const std::string &d, const std::string &c, const std::string &s, const std::string &co)
			: headline(h), date(d), creator(c), subject(s), comments(co) {}
};

class widgetRSSFeeds : public ui_window
{
	public:
		bool redraw(uint8_t fade_amount, int8_t tab_group = -1) override;
		bool process_touch(touch_event_t touch_event);
		void process_article_data(bool success, const String &response);
		void show_next_article();

	private:
		std::string feed_url = "";
		unsigned long next_update = 0;
		unsigned long next_article_swap = 0;

		bool is_debug = true;

		bool should_redraw = false;
		bool is_setup = false;
		bool process_next_article = false;
		bool is_getting_more_articles = false;
		bool has_had_any_articles = false;

		uint8_t char_width = 0;
		uint8_t char_height = 0;
		uint8_t max_chars_per_line = 0;
		uint8_t max_lines = 0;

		std::vector<String> lines;

		std::vector<ARTICLE> stored_articles;

		BB_SPI_LCD _sprite_article;

		void process_lines();
		void reset_refresh_timer();

		unsigned long parse_date(const std::string &date_str);
		std::string format_time_ago(unsigned long seconds_ago);
};

extern widgetRSSFeeds widget_rss_feeds;