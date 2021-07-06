#pragma once
#ifndef ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_H
#define ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_H

#include "scrapers/Scraper.h"

namespace pugi
{
class xml_document;
}

void thegamesdb_generate_json_scraper_requests(const ScraperSearchParams& params,
	std::queue<std::unique_ptr<ScraperRequest>>& requests, std::vector<ScraperSearchResult>& results);

class TheGamesDBJSONRequest : public ScraperHttpRequest
{
  public:
	// ctor for a GetGameList request
	TheGamesDBJSONRequest(std::queue<std::unique_ptr<ScraperRequest>>& requestsWrite,
		std::vector<ScraperSearchResult>& resultsWrite, const std::string& url)
		: ScraperHttpRequest(resultsWrite, url), mRequestQueue(&requestsWrite)
	{
	}
	// ctor for a GetGame request
	TheGamesDBJSONRequest(std::vector<ScraperSearchResult>& resultsWrite, const std::string& url)
		: ScraperHttpRequest(resultsWrite, url), mRequestQueue(nullptr)
	{
	}

  protected:
	void process(const std::unique_ptr<HttpReq>& req, std::vector<ScraperSearchResult>& results) override;
	bool isGameRequest() { return !mRequestQueue; }

	std::queue<std::unique_ptr<ScraperRequest>>* mRequestQueue;
};

#endif // ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_H
