#pragma once
#ifndef ES_APP_COMPONENTS_ASYNC_REQ_COMPONENT_H
#define ES_APP_COMPONENTS_ASYNC_REQ_COMPONENT_H

#include "GuiComponent.h"

class HttpReq;

/*
	Used to asynchronously run an HTTP request.
	Displays a simple animation on the UI to show the application hasn't frozen.  Can be canceled by the user pressing B.

	Usage example:
		std::shared_ptr<HttpReq> httpreq = std::make_shared<HttpReq>("cdn.garcya.us", "/wp-content/uploads/2010/04/TD250.jpg");
		AsyncReqComponent* req = new AsyncReqComponent(mWindow, httpreq,
			[] (std::shared_ptr<HttpReq> r)
		{
			LOG(LogInfo) << "Request completed";
			LOG(LogInfo) << "   error, if any: " << r->getErrorMsg();
		}, [] ()
		{
			LOG(LogInfo) << "Request canceled";
		});

		mWindow->pushGui(req);
		//we can forget about req, since it will always delete itself
*/

class AsyncReqComponent : public GuiComponent
{
public:

	AsyncReqComponent(Window* window, std::shared_ptr<HttpReq> req, std::function<void(std::shared_ptr<HttpReq>)> onSuccess, std::function<void()> onCancel = nullptr);

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;

	virtual std::vector<HelpPrompt> getHelpPrompts() override;
private:
	std::function<void(std::shared_ptr<HttpReq>)> mSuccessFunc;
	std::function<void()> mCancelFunc;

	unsigned int mTime;
	std::shared_ptr<HttpReq> mRequest;
};

#endif // ES_APP_COMPONENTS_ASYNC_REQ_COMPONENT_H
