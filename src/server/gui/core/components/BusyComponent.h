#pragma once
#ifndef ES_CORE_COMPONENTS_BUSY_COMPONENT_H
#define ES_CORE_COMPONENTS_BUSY_COMPONENT_H

#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"
#include "GuiComponent.h"

class AnimatedImageComponent;
class TextComponent;

class BusyComponent : public GuiComponent
{
public:
	BusyComponent(Window* window);

	void onSizeChanged() override;

	void reset(); // reset to frame 0

private:
	NinePatchComponent mBackground;
	ComponentGrid mGrid;

	std::shared_ptr<AnimatedImageComponent> mAnimation;
	std::shared_ptr<TextComponent> mText;
};

#endif // ES_CORE_COMPONENTS_BUSY_COMPONENT_H
