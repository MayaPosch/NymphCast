#pragma once
#ifndef ES_CORE_COMPONENTS_NINE_PATCH_COMPONENT_H
#define ES_CORE_COMPONENTS_NINE_PATCH_COMPONENT_H

#include "renderers/Renderer.h"
#include "GuiComponent.h"

class TextureResource;

// Display an image in a way so that edges don't get too distorted no matter the final size. Useful for UI elements like backgrounds, buttons, etc.
// This is accomplished by splitting an image into 9 pieces:
//  ___________
// |_1_|_2_|_3_|
// |_4_|_5_|_6_|
// |_7_|_8_|_9_|

// Corners (1, 3, 7, 9) will not be stretched at all.
// Borders (2, 4, 6, 8) will be stretched along one axis (2 and 8 horizontally, 4 and 6 vertically).
// The center (5) will be stretched.

class NinePatchComponent : public GuiComponent
{
public:
	NinePatchComponent(Window* window, const std::string& path = "", unsigned int edgeColor = 0xFFFFFFFF, unsigned int centerColor = 0xFFFFFFFF);
	virtual ~NinePatchComponent();

	void render(const Transform4x4f& parentTrans) override;

	void onSizeChanged() override;

	void fitTo(Vector2f size, Vector3f position = Vector3f::Zero(), Vector2f padding = Vector2f::Zero());

	void setImagePath(const std::string& path);
	void setEdgeColor(unsigned int edgeColor); // Apply a color shift to the "edge" parts of the ninepatch.
	void setCenterColor(unsigned int centerColor); // Apply a color shift to the "center" part of the ninepatch.

	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

	const Vector2f& getCornerSize() const;
	void setCornerSize(int sizeX, int sizeY);
	inline void setCornerSize(const Vector2f& size) { setCornerSize(size.x(), size.y()); }

private:
	void buildVertices();
	void updateColors();

	Renderer::Vertex* mVertices;

	std::string mPath;
	Vector2f mCornerSize;
	unsigned int mEdgeColor;
	unsigned int mCenterColor;
	std::shared_ptr<TextureResource> mTexture;
};

#endif // ES_CORE_COMPONENTS_NINE_PATCH_COMPONENT_H
