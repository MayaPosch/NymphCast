#pragma once
#ifndef ES_CORE_GUI_COMPONENT_H
#define ES_CORE_GUI_COMPONENT_H

#include "math/Misc.h"
#include "math/Transform4x4f.h"
#include "HelpPrompt.h"
#include "HelpStyle.h"
#include "InputConfig.h"
#include <functional>
#include <memory>

class Animation;
class AnimationController;
class Font;
class InputConfig;
class ThemeData;
class Window;

class GuiComponent
{
public:
	GuiComponent(Window* window);
	virtual ~GuiComponent();

	virtual void textInput(const char* text);

	//Called when input is received.
	//Return true if the input is consumed, false if it should continue to be passed to other children.
	virtual bool input(InputConfig* config, Input input);

	//Called when time passes.  Default implementation calls updateSelf(deltaTime) and updateChildren(deltaTime) - so you should probably call GuiComponent::update(deltaTime) at some point (or at least updateSelf so animations work).
	virtual void update(int deltaTime);

	//Called when it's time to render.  By default, just calls renderChildren(parentTrans * getTransform()).
	//You probably want to override this like so:
	//1. Calculate the new transform that your control will draw at with Transform4x4f t = parentTrans * getTransform().
	//2. Set the renderer to use that new transform as the model matrix - Renderer::setMatrix(t);
	//3. Draw your component.
	//4. Tell your children to render, based on your component's transform - renderChildren(t).
	virtual void render(const Transform4x4f& parentTrans);

	Vector3f getPosition() const;
	inline void setPosition(const Vector3f& offset) { setPosition(offset.x(), offset.y(), offset.z()); }
	void setPosition(float x, float y, float z = 0.0f);
	virtual void onPositionChanged() {};

	//Sets the origin as a percentage of this image (e.g. (0, 0) is top left, (0.5, 0.5) is the center)
	Vector2f getOrigin() const;
	void setOrigin(float originX, float originY);
	inline void setOrigin(Vector2f origin) { setOrigin(origin.x(), origin.y()); }
	virtual void onOriginChanged() {};

	//Sets the rotation origin as a percentage of this image (e.g. (0, 0) is top left, (0.5, 0.5) is the center)
	Vector2f getRotationOrigin() const;
	void setRotationOrigin(float originX, float originY);
	inline void setRotationOrigin(Vector2f origin) { setRotationOrigin(origin.x(), origin.y()); }

	virtual Vector2f getSize() const;
    inline void setSize(const Vector2f& size) { setSize(size.x(), size.y()); }
    void setSize(float w, float h);
    virtual void onSizeChanged() {};

	virtual Vector2f getRotationSize() const { return getSize(); };

	float getRotation() const;
	void setRotation(float rotation);
	inline void setRotationDegrees(float rotation) { setRotation((float)ES_DEG_TO_RAD(rotation)); }

	float getScale() const;
	void setScale(float scale);

    float getZIndex() const;
    void setZIndex(float zIndex);

    float getDefaultZIndex() const;
    void setDefaultZIndex(float zIndex);

    bool isVisible() const;
    void setVisible(bool visible);

	// Returns the center point of the image (takes origin into account).
	Vector2f getCenter() const;

	void setParent(GuiComponent* parent);
	GuiComponent* getParent() const;

	void addChild(GuiComponent* cmp);
	void removeChild(GuiComponent* cmp);
	void clearChildren();
	void sortChildren();
	unsigned int getChildCount() const;
	GuiComponent* getChild(unsigned int i) const;

	// animation will be automatically deleted when it completes or is stopped.
	bool isAnimationPlaying(unsigned char slot) const;
	bool isAnimationReversed(unsigned char slot) const;
	int getAnimationTime(unsigned char slot) const;
	void setAnimation(Animation* animation, int delay = 0, std::function<void()> finishedCallback = nullptr, bool reverse = false, unsigned char slot = 0);
	bool stopAnimation(unsigned char slot);
	bool cancelAnimation(unsigned char slot); // Like stopAnimation, but doesn't call finishedCallback - only removes the animation, leaving things in their current state.  Returns true if successful (an animation was in this slot).
	bool finishAnimation(unsigned char slot); // Calls update(1.f) and finishedCallback, then deletes the animation - basically skips to the end.  Returns true if successful (an animation was in this slot).
	bool advanceAnimation(unsigned char slot, unsigned int time); // Returns true if successful (an animation was in this slot).
	void stopAllAnimations();
	void cancelAllAnimations();

	virtual unsigned char getOpacity() const;
	virtual void setOpacity(unsigned char opacity);

	const Transform4x4f& getTransform();

	virtual std::string getValue() const;
	virtual void setValue(const std::string& value);

	virtual void onFocusGained() {};
	virtual void onFocusLost() {};

	virtual void onShow();
	virtual void onHide();

	virtual void onScreenSaverActivate();
	virtual void onScreenSaverDeactivate();
	virtual void topWindow(bool isTop);

	// Default implementation just handles <pos> and <size> tags as normalized float pairs.
	// You probably want to keep this behavior for any derived classes as well as add your own.
	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties);

	// Returns a list of help prompts.
	virtual std::vector<HelpPrompt> getHelpPrompts() { return std::vector<HelpPrompt>(); };

	// Called whenever help prompts change.
	void updateHelpPrompts();

	virtual HelpStyle getHelpStyle();

	// Returns true if the component is busy doing background processing (e.g. HTTP downloads)
	bool isProcessing() const;

protected:
	void renderChildren(const Transform4x4f& transform) const;
	void updateSelf(int deltaTime); // updates animations
	void updateChildren(int deltaTime); // updates animations

	unsigned char mOpacity;
	Window* mWindow;

	GuiComponent* mParent;
	std::vector<GuiComponent*> mChildren;

	Vector3f mPosition;
	Vector2f mOrigin;
	Vector2f mRotationOrigin;
	Vector2f mSize;

	float mRotation = 0.0;
	float mScale = 1.0;

	float mDefaultZIndex = 0;
	float mZIndex = 0;

	bool mIsProcessing;
	bool mVisible;

public:
	const static unsigned char MAX_ANIMATIONS = 4;

private:
	Transform4x4f mTransform; //Don't access this directly! Use getTransform()!
	AnimationController* mAnimationMap[MAX_ANIMATIONS];
};

#endif // ES_CORE_GUI_COMPONENT_H
