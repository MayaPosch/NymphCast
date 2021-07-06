#include "GridTileComponent.h"

#include "animations/LambdaAnimation.h"
#include "resources/TextureResource.h"
#include "ThemeData.h"

GridTileComponent::GridTileComponent(Window* window) : GuiComponent(window), mBackground(window, ":/frame.png")
{
	mDefaultProperties.mSize = getDefaultTileSize();
	mDefaultProperties.mPadding = Vector2f(16.0f, 16.0f);
	mDefaultProperties.mImageColor = 0xAAAAAABB;
	mDefaultProperties.mBackgroundImage = ":/frame.png";
	mDefaultProperties.mBackgroundCornerSize = Vector2f(16 ,16);
	mDefaultProperties.mBackgroundCenterColor = 0xAAAAEEFF;
	mDefaultProperties.mBackgroundEdgeColor = 0xAAAAEEFF;

	mSelectedProperties.mSize = getSelectedTileSize();
	mSelectedProperties.mPadding = mDefaultProperties.mPadding;
	mSelectedProperties.mImageColor = 0xFFFFFFFF;
	mSelectedProperties.mBackgroundImage = mDefaultProperties.mBackgroundImage;
	mSelectedProperties.mBackgroundCornerSize = mDefaultProperties.mBackgroundCornerSize;
	mSelectedProperties.mBackgroundCenterColor = 0xFFFFFFFF;
	mSelectedProperties.mBackgroundEdgeColor = 0xFFFFFFFF;

	mImage = std::make_shared<ImageComponent>(mWindow);
	mImage->setOrigin(0.5f, 0.5f);

	mBackground.setOrigin(0.5f, 0.5f);

	addChild(&mBackground);
	addChild(&(*mImage));

	mSelectedZoomPercent = 0;

	setSelected(false, false);
	setVisible(true);
}

void GridTileComponent::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = getTransform() * parentTrans;

	if (mVisible)
		renderChildren(trans);
}

// Update all the tile properties to the new status (selected or default)
void GridTileComponent::update(int deltaTime)
{
	GuiComponent::update(deltaTime);

	calcCurrentProperties();

	mBackground.setImagePath(mCurrentProperties.mBackgroundImage);

	mImage->setColorShift(mCurrentProperties.mImageColor);
	mBackground.setCenterColor(mCurrentProperties.mBackgroundCenterColor);
	mBackground.setEdgeColor(mCurrentProperties.mBackgroundEdgeColor);

	resize();
}

void applyThemeToProperties(const ThemeData::ThemeElement* elem, GridTileProperties* properties)
{
	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	if (elem->has("size"))
		properties->mSize = elem->get<Vector2f>("size") * screen;

	if (elem->has("padding"))
	{
		properties->mPadding = elem->get<Vector2f>("padding") * screen;

		// hack to fix broken themes now that this uses percentage rather than pixels
		if(properties->mPadding.x() > screen.x())
			properties->mPadding /= screen;
	}

	if (elem->has("imageColor"))
		properties->mImageColor = elem->get<unsigned int>("imageColor");

	if (elem->has("backgroundImage"))
		properties->mBackgroundImage = elem->get<std::string>("backgroundImage");

	if (elem->has("backgroundCornerSize"))
	{
		properties->mBackgroundCornerSize = elem->get<Vector2f>("backgroundCornerSize") * screen;

		// hack to fix broken themes now that this uses percentage rather than pixels
		if(properties->mBackgroundCornerSize.x() > screen.x())
			properties->mBackgroundCornerSize /= screen;
	}

	if (elem->has("backgroundColor"))
	{
		properties->mBackgroundCenterColor = elem->get<unsigned int>("backgroundColor");
		properties->mBackgroundEdgeColor = elem->get<unsigned int>("backgroundColor");
	}

	if (elem->has("backgroundCenterColor"))
		properties->mBackgroundCenterColor = elem->get<unsigned int>("backgroundCenterColor");

	if (elem->has("backgroundEdgeColor"))
		properties->mBackgroundEdgeColor = elem->get<unsigned int>("backgroundEdgeColor");
}

void GridTileComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& /*element*/, unsigned int /*properties*/)
{
	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	// Apply theme to the default gridtile
	const ThemeData::ThemeElement* elem = theme->getElement(view, "default", "gridtile");
	if (elem)
		applyThemeToProperties(elem, &mDefaultProperties);

	// Apply theme to the selected gridtile
	// NOTE that some of the default gridtile properties influence on the selected gridtile properties
	// See THEMES.md for more informations
	elem = theme->getElement(view, "selected", "gridtile");

	mSelectedProperties.mSize = getSelectedTileSize();
	mSelectedProperties.mPadding = mDefaultProperties.mPadding;
	mSelectedProperties.mBackgroundImage = mDefaultProperties.mBackgroundImage;
	mSelectedProperties.mBackgroundCornerSize = mDefaultProperties.mBackgroundCornerSize;

	if (elem)
		applyThemeToProperties(elem, &mSelectedProperties);
}

// Made this a static function because the ImageGridComponent need to know the default tile size
// to calculate the grid dimension before it instantiate the GridTileComponents
Vector2f GridTileComponent::getDefaultTileSize()
{
	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	return screen * 0.22f;
}

Vector2f GridTileComponent::getSelectedTileSize() const
{
	return mDefaultProperties.mSize * 1.2f;
}

bool GridTileComponent::isSelected() const
{
	return mSelected;
}

void GridTileComponent::reset()
{
	setImage("");
}

void GridTileComponent::setImage(const std::string& path)
{
	mImage->setImage(path);

	// Resize now to prevent flickering images when scrolling
	resize();
}

void GridTileComponent::setImage(const std::shared_ptr<TextureResource>& texture)
{
	mImage->setImage(texture);

	// Resize now to prevent flickering images when scrolling
	resize();
}

void GridTileComponent::setSelected(bool selected, bool allowAnimation, Vector3f* pPosition, bool force)
{
	if (mSelected == selected && !force)
	{
		return;
	}

	mSelected = selected;

	if (selected)
	{
		if (pPosition == NULL || !allowAnimation)
		{
			cancelAnimation(3);

			this->setSelectedZoom(1);
			mAnimPosition = Vector3f(0, 0, 0);

			resize();
		}
		else
		{
			mAnimPosition = Vector3f(pPosition->x(), pPosition->y(), pPosition->z());

			auto func = [this](float t)
			{
				t -= 1; // cubic ease out
				float pct = Math::lerp(0, 1, t*t*t + 1);

				this->setSelectedZoom(pct);
			};

			cancelAnimation(3);
			setAnimation(new LambdaAnimation(func, 250), 0, [this] {
				this->setSelectedZoom(1);
				mAnimPosition = Vector3f(0, 0, 0);
			}, false, 3);
		}
	}
	else // if (!selected)
	{
		if (!allowAnimation)
		{
			cancelAnimation(3);
			this->setSelectedZoom(0);

			resize();
		}
		else
		{
			this->setSelectedZoom(1);

			auto func = [this](float t)
			{
				t -= 1; // cubic ease out
				float pct = Math::lerp(0, 1, t*t*t + 1);
				this->setSelectedZoom(1.0 - pct);
			};

			cancelAnimation(3);
			setAnimation(new LambdaAnimation(func, 250), 0, [this] {
				this->setSelectedZoom(0);
			}, false, 3);
		}
	}
}

void GridTileComponent::setSelectedZoom(float percent)
{
	if (mSelectedZoomPercent == percent)
		return;

	mSelectedZoomPercent = percent;
	resize();
}

void GridTileComponent::setVisible(bool visible)
{
	mVisible = visible;
}

void GridTileComponent::resize()
{
	calcCurrentProperties();

	mImage->setMaxSize(mCurrentProperties.mSize - mCurrentProperties.mPadding * 2);
	mBackground.setCornerSize(mCurrentProperties.mBackgroundCornerSize);
	mBackground.fitTo(mCurrentProperties.mSize - mBackground.getCornerSize() * 2);
}

unsigned int mixColors(unsigned int first, unsigned int second, float percent)
{
	unsigned char alpha0 = (first >> 24) & 0xFF;
	unsigned char blue0 = (first >> 16) & 0xFF;
	unsigned char green0 = (first >> 8) & 0xFF;
	unsigned char red0 = first & 0xFF;

	unsigned char alpha1 = (second >> 24) & 0xFF;
	unsigned char blue1 = (second >> 16) & 0xFF;
	unsigned char green1 = (second >> 8) & 0xFF;
	unsigned char red1 = second & 0xFF;

	unsigned char alpha = (unsigned char)(alpha0 * (1.0 - percent) + alpha1 * percent);
	unsigned char blue = (unsigned char)(blue0 * (1.0 - percent) + blue1 * percent);
	unsigned char green = (unsigned char)(green0 * (1.0 - percent) + green1 * percent);
	unsigned char red = (unsigned char)(red0 * (1.0 - percent) + red1 * percent);

	return (alpha << 24) | (blue << 16) | (green << 8) | red;
}

void GridTileComponent::calcCurrentProperties()
{
	mCurrentProperties = mSelected ? mSelectedProperties : mDefaultProperties;

	float zoomPercentInverse = 1.0 - mSelectedZoomPercent;

	if (mSelectedZoomPercent != 0.0f && mSelectedZoomPercent != 1.0f) {
		if (mDefaultProperties.mSize != mSelectedProperties.mSize) {
			mCurrentProperties.mSize = mDefaultProperties.mSize * zoomPercentInverse + mSelectedProperties.mSize * mSelectedZoomPercent;
		}

		if (mDefaultProperties.mPadding != mSelectedProperties.mPadding)
		{
			mCurrentProperties.mPadding = mDefaultProperties.mPadding * zoomPercentInverse + mSelectedProperties.mPadding * mSelectedZoomPercent;
		}

		if (mDefaultProperties.mImageColor != mSelectedProperties.mImageColor)
		{
			mCurrentProperties.mImageColor = mixColors(mDefaultProperties.mImageColor, mSelectedProperties.mImageColor, mSelectedZoomPercent);
		}

		if (mDefaultProperties.mBackgroundCornerSize != mSelectedProperties.mBackgroundCornerSize)
		{
			mCurrentProperties.mBackgroundCornerSize = mDefaultProperties.mBackgroundCornerSize * zoomPercentInverse + mSelectedProperties.mBackgroundCornerSize * mSelectedZoomPercent;
		}

		if (mDefaultProperties.mBackgroundCenterColor != mSelectedProperties.mBackgroundCenterColor)
		{
			mCurrentProperties.mBackgroundCenterColor = mixColors(mDefaultProperties.mBackgroundCenterColor, mSelectedProperties.mBackgroundCenterColor, mSelectedZoomPercent);
		}

		if (mDefaultProperties.mBackgroundEdgeColor != mSelectedProperties.mBackgroundEdgeColor)
		{
			mCurrentProperties.mBackgroundEdgeColor = mixColors(mDefaultProperties.mBackgroundEdgeColor, mSelectedProperties.mBackgroundEdgeColor, mSelectedZoomPercent);
		}
	}
}

Vector3f GridTileComponent::getBackgroundPosition()
{
	return mBackground.getPosition() + mPosition;
}

std::shared_ptr<TextureResource> GridTileComponent::getTexture()
{
	if (mImage != nullptr)
		return mImage->getTexture();

	return nullptr;
};

void GridTileComponent::forceSize(Vector2f size, float selectedZoom)
{
	mDefaultProperties.mSize = size;
	mSelectedProperties.mSize = size * selectedZoom;
}
