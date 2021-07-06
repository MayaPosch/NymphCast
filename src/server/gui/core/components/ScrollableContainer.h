#pragma once
#ifndef ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H
#define ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H

#include "GuiComponent.h"

class ScrollableContainer : public GuiComponent
{
public:
	ScrollableContainer(Window* window, int scrollDelay = 0);

	Vector2f getScrollPos() const;
	void setScrollPos(const Vector2f& pos);
	void setAutoScroll(bool autoScroll);
	void reset();

	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;

private:
	Vector2f getContentSize();

	Vector2f mScrollPos;
	Vector2f mScrollDir;
	int mAutoScrollDelay; // ms to wait before starting to autoscroll
	int mAutoScrollSpeed; // ms to wait before scrolling down by mScrollDir
	int mAutoScrollAccumulator;
	bool mAtEnd;
	int mAutoScrollResetAccumulator;
};

#endif // ES_CORE_COMPONENTS_SCROLLABLE_CONTAINER_H
