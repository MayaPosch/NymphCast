#pragma once
#ifndef ES_CORE_HTTP_REQ_H
#define ES_CORE_HTTP_REQ_H

#include <curl/curl.h>
#include <map>
#include <sstream>

/* Usage:
 * HttpReq myRequest("www.google.com", "/index.html");
 * //for blocking behavior: while(myRequest.status() == HttpReq::REQ_IN_PROGRESS);
 * //for non-blocking behavior: check if(myRequest.status() != HttpReq::REQ_IN_PROGRESS) in some sort of update method
 *
 * //once one of those completes, the request is ready
 * if(myRequest.status() != REQ_SUCCESS)
 * {
 *    //an error occured
 *    LOG(LogError) << "HTTP request error - " << myRequest.getErrorMessage();
 *    return;
 * }
 *
 * std::string content = myRequest.getContent();
 * //process contents...
*/

class HttpReq
{
public:
	HttpReq(const std::string& url);

	~HttpReq();

	enum Status
	{
		REQ_IN_PROGRESS,		//request is in progress
		REQ_SUCCESS,			//request completed successfully, get it with getContent()

		REQ_IO_ERROR,			//some error happened, get it with getErrorMsg()
		REQ_BAD_STATUS_CODE,	//some invalid HTTP response status code happened (non-200)
		REQ_INVALID_RESPONSE	//the HTTP response was invalid
	};

	Status status(); //process any received data and return the status afterwards

	std::string getErrorMsg();

	std::string getContent() const; // mStatus must be REQ_SUCCESS

	static std::string urlEncode(const std::string &s);
	static bool isUrl(const std::string& s);

private:
	static size_t write_content(void* buff, size_t size, size_t nmemb, void* req_ptr);
	//static int update_progress(void* req_ptr, double dlTotal, double dlNow, double ulTotal, double ulNow);

	//god dammit libcurl why can't you have some way to check the status of an individual handle
	//why do I have to handle ALL messages at once
	static std::map<CURL*, HttpReq*> s_requests;

	static CURLM* s_multi_handle;

	void onError(const char* msg);

	CURL* mHandle;

	Status mStatus;

	std::stringstream mContent;
	std::string mErrorMsg;
};

#endif // ES_CORE_HTTP_REQ_H
