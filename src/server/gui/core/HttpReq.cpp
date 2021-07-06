#include "HttpReq.h"

#include "utils/FileSystemUtil.h"
#include "Log.h"
#include <assert.h>

CURLM* HttpReq::s_multi_handle = curl_multi_init();

std::map<CURL*, HttpReq*> HttpReq::s_requests;

std::string HttpReq::urlEncode(const std::string &s)
{
    const std::string unreserved = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~";

    std::string escaped="";
    for(size_t i=0; i<s.length(); i++)
    {
        if (unreserved.find_first_of(s[i]) != std::string::npos)
        {
            escaped.push_back(s[i]);
        }
        else
        {
            escaped.append("%");
            char buf[3];
            sprintf(buf, "%.2X", (unsigned char)s[i]);
            escaped.append(buf);
        }
    }
    return escaped;
}

bool HttpReq::isUrl(const std::string& str)
{
	//the worst guess
	return (!str.empty() && !Utils::FileSystem::exists(str) &&
		(str.find("http://") != std::string::npos || str.find("https://") != std::string::npos || str.find("www.") != std::string::npos));
}

HttpReq::HttpReq(const std::string& url)
	: mStatus(REQ_IN_PROGRESS), mHandle(NULL)
{
	mHandle = curl_easy_init();

	if(mHandle == NULL)
	{
		mStatus = REQ_IO_ERROR;
		onError("curl_easy_init failed");
		return;
	}

	//set the url
	CURLcode err = curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str());
	if(err != CURLE_OK)
	{
		mStatus = REQ_IO_ERROR;
		onError(curl_easy_strerror(err));
		return;
	}

	//set curl to handle redirects
	err = curl_easy_setopt(mHandle, CURLOPT_FOLLOWLOCATION, 1L);
	if(err != CURLE_OK)
	{
		mStatus = REQ_IO_ERROR;
		onError(curl_easy_strerror(err));
		return;
	}

	//set curl max redirects
	err = curl_easy_setopt(mHandle, CURLOPT_MAXREDIRS, 2L);
	if(err != CURLE_OK)
	{
		mStatus = REQ_IO_ERROR;
		onError(curl_easy_strerror(err));
		return;
	}

	//set curl restrict redirect protocols
	err = curl_easy_setopt(mHandle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
	if(err != CURLE_OK)
	{
		mStatus = REQ_IO_ERROR;
		onError(curl_easy_strerror(err));
		return;
	}

	//tell curl how to write the data
	err = curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, &HttpReq::write_content);
	if(err != CURLE_OK)
	{
		mStatus = REQ_IO_ERROR;
		onError(curl_easy_strerror(err));
		return;
	}

	//give curl a pointer to this HttpReq so we know where to write the data *to* in our write function
	err = curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, this);
	if(err != CURLE_OK)
	{
		mStatus = REQ_IO_ERROR;
		onError(curl_easy_strerror(err));
		return;
	}

	//add the handle to our multi
	CURLMcode merr = curl_multi_add_handle(s_multi_handle, mHandle);
	if(merr != CURLM_OK)
	{
		mStatus = REQ_IO_ERROR;
		onError(curl_multi_strerror(merr));
		return;
	}

	s_requests[mHandle] = this;
}

HttpReq::~HttpReq()
{
	if(mHandle)
	{
		s_requests.erase(mHandle);

		CURLMcode merr = curl_multi_remove_handle(s_multi_handle, mHandle);

		if(merr != CURLM_OK)
			LOG(LogError) << "Error removing curl_easy handle from curl_multi: " << curl_multi_strerror(merr);

		curl_easy_cleanup(mHandle);
	}
}

HttpReq::Status HttpReq::status()
{
	if(mStatus == REQ_IN_PROGRESS)
	{
		int handle_count;
		CURLMcode merr = curl_multi_perform(s_multi_handle, &handle_count);
		if(merr != CURLM_OK && merr != CURLM_CALL_MULTI_PERFORM)
		{
			mStatus = REQ_IO_ERROR;
			onError(curl_multi_strerror(merr));
			return mStatus;
		}

		int msgs_left;
		CURLMsg* msg;
		while((msg = curl_multi_info_read(s_multi_handle, &msgs_left)) != nullptr)
		{
			if(msg->msg == CURLMSG_DONE)
			{
				HttpReq* req = s_requests[msg->easy_handle];

				if(req == NULL)
				{
					LOG(LogError) << "Cannot find easy handle!";
					continue;
				}

				if(msg->data.result == CURLE_OK)
				{
					req->mStatus = REQ_SUCCESS;
				}else{
					req->mStatus = REQ_IO_ERROR;
					req->onError(curl_easy_strerror(msg->data.result));
				}
			}
		}
	}

	return mStatus;
}

std::string HttpReq::getContent() const
{
	assert(mStatus == REQ_SUCCESS);
	return mContent.str();
}

void HttpReq::onError(const char* msg)
{
	mErrorMsg = msg;
}

std::string HttpReq::getErrorMsg()
{
	return mErrorMsg;
}

//used as a curl callback
//size = size of an element, nmemb = number of elements
//return value is number of elements successfully read
size_t HttpReq::write_content(void* buff, size_t size, size_t nmemb, void* req_ptr)
{
	std::stringstream& ss = ((HttpReq*)req_ptr)->mContent;
	ss.write((char*)buff, size * nmemb);

	return nmemb;
}

//used as a curl callback
/*int HttpReq::update_progress(void* req_ptr, double dlTotal, double dlNow, double ulTotal, double ulNow)
{

}*/
