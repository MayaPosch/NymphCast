#pragma once
#ifndef ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H
#define ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H

#include "Log.h"
#include "animations/LambdaAnimation.h"
#include "components/IList.h"
#include "resources/TextureResource.h"
#include "GridTileComponent.h"

#define EXTRAITEMS 2

enum ScrollDirection
{
	SCROLL_VERTICALLY,
	SCROLL_HORIZONTALLY
};

enum ImageSource
{
	THUMBNAIL,
	IMAGE,
	MARQUEE
};

struct ImageGridData
{
	std::string texturePath;
};

template<typename T>
class ImageGridComponent : public IList<ImageGridData, T>
{
protected:
	using IList<ImageGridData, T>::mEntries;
	using IList<ImageGridData, T>::mScrollTier;
	using IList<ImageGridData, T>::listUpdate;
	using IList<ImageGridData, T>::listInput;
	using IList<ImageGridData, T>::listRenderTitleOverlay;
	using IList<ImageGridData, T>::getTransform;
	using IList<ImageGridData, T>::mSize;
	using IList<ImageGridData, T>::mCursor;
	using IList<ImageGridData, T>::Entry;
	using IList<ImageGridData, T>::mWindow;

public:
	using IList<ImageGridData, T>::size;
	using IList<ImageGridData, T>::isScrolling;
	using IList<ImageGridData, T>::stopScrolling;

	ImageGridComponent(Window* window);

	void add(const std::string& name, const std::string& imagePath, const T& obj);

	bool input(InputConfig* config, Input input) override;
	void update(int deltaTime) override;
	void render(const Transform4x4f& parentTrans) override;
	virtual void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties) override;

	void onSizeChanged() override;
	inline void setCursorChangedCallback(const std::function<void(CursorState state)>& func) { mCursorChangedCallback = func; }

	ImageSource	getImageSource() { return mImageSource; };

protected:
	virtual void onCursorChanged(const CursorState& state) override;

private:
	// TILES
	void buildTiles();
	void updateTiles(bool allowAnimation = true, bool updateSelectedState = true);
	void updateTileAtPos(int tilePos, int imgPos, bool allowAnimation, bool updateSelectedState);
	void calcGridDimension();
	bool isScrollLoop();

	bool isVertical() { return mScrollDirection == SCROLL_VERTICALLY; };


	// IMAGES & ENTRIES
	bool mEntriesDirty;
	int mLastCursor;
	std::string mDefaultGameTexture;
	std::string mDefaultFolderTexture;

	// TILES
	bool mLastRowPartial;
	Vector2f mAutoLayout;
	float mAutoLayoutZoom;

	Vector4f mPadding;
	Vector2f mMargin;
	Vector2f mTileSize;
	Vector2i mGridDimension;
	std::shared_ptr<ThemeData> mTheme;
	std::vector< std::shared_ptr<GridTileComponent> > mTiles;

	int mStartPosition;

	float mCamera;
	float mCameraDirection;

	// MISCELLANEOUS
	bool mAnimate;
	bool mCenterSelection;
	bool mScrollLoop;
	ScrollDirection mScrollDirection;
	ImageSource mImageSource;
	std::function<void(CursorState state)> mCursorChangedCallback;
};

template<typename T>
ImageGridComponent<T>::ImageGridComponent(Window* window) : IList<ImageGridData, T>(window)
{
	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	mCamera = 0.0;
	mCameraDirection = 1.0;

	mAutoLayout = Vector2f::Zero();
	mAutoLayoutZoom = 1.0;

	mStartPosition = 0;

	mEntriesDirty = true;
	mLastCursor = 0;
	mDefaultGameTexture = ":/cartridge.svg";
	mDefaultFolderTexture = ":/folder.svg";

	mSize = screen * 0.80f;
	mMargin = screen * 0.07f;
	mPadding = Vector4f::Zero();
	mTileSize = GridTileComponent::getDefaultTileSize();

	mAnimate = true;
	mCenterSelection = false;
	mScrollLoop = false;
	mScrollDirection = SCROLL_VERTICALLY;
	mImageSource = THUMBNAIL;
}

template<typename T>
void ImageGridComponent<T>::add(const std::string& name, const std::string& imagePath, const T& obj)
{
	typename IList<ImageGridData, T>::Entry entry;
	entry.name = name;
	entry.object = obj;
	entry.data.texturePath = imagePath;

	static_cast<IList< ImageGridData, T >*>(this)->add(entry);
	mEntriesDirty = true;
}

template<typename T>
bool ImageGridComponent<T>::input(InputConfig* config, Input input)
{
	if(input.value != 0)
	{
		int idx = isVertical() ? 0 : 1;

		Vector2i dir = Vector2i::Zero();
		if(config->isMappedLike("up", input))
			dir[1 ^ idx] = -1;
		else if(config->isMappedLike("down", input))
			dir[1 ^ idx] = 1;
		else if(config->isMappedLike("left", input))
			dir[0 ^ idx] = -1;
		else if(config->isMappedLike("right", input))
			dir[0 ^ idx] = 1;

		if(dir != Vector2i::Zero())
		{
			if (isVertical())
				listInput(dir.x() + dir.y() * mGridDimension.x());
			else
				listInput(dir.x() + dir.y() * mGridDimension.y());
			return true;
		}
	}else{
		if(config->isMappedLike("up", input) || config->isMappedLike("down", input) || config->isMappedLike("left", input) || config->isMappedLike("right", input))
		{
			stopScrolling();
		}
	}

	return GuiComponent::input(config, input);
}

template<typename T>
void ImageGridComponent<T>::update(int deltaTime)
{
	GuiComponent::update(deltaTime);
	listUpdate(deltaTime);

	for(auto it = mTiles.begin(); it != mTiles.end(); it++)
		(*it)->update(deltaTime);
}

template<typename T>
void ImageGridComponent<T>::render(const Transform4x4f& parentTrans)
{
	Transform4x4f trans = getTransform() * parentTrans;
	Transform4x4f tileTrans = trans;

	float offsetX = isVertical() ? 0.0f : mCamera * mCameraDirection * (mTileSize.x() + mMargin.x());
	float offsetY = isVertical() ? mCamera * mCameraDirection * (mTileSize.y() + mMargin.y()) : 0.0f;

	tileTrans.translate(Vector3f(offsetX, offsetY, 0.0));

	if(mEntriesDirty)
	{
		updateTiles();
		mEntriesDirty = false;
	}

	// Create a clipRect to hide tiles used to buffer texture loading
	float scaleX = trans.r0().x();
	float scaleY = trans.r1().y();

	Vector2i pos((int)Math::round(trans.translation()[0]), (int)Math::round(trans.translation()[1]));
	Vector2i size((int)Math::round(mSize.x() * scaleX), (int)Math::round(mSize.y() * scaleY));

	Renderer::pushClipRect(pos, size);

	// Render all the tiles but the selected one
	std::shared_ptr<GridTileComponent> selectedTile = NULL;
	for(auto it = mTiles.begin(); it != mTiles.end(); it++)
	{
		std::shared_ptr<GridTileComponent> tile = (*it);

		// If it's the selected image, keep it for later, otherwise render it now
		if(tile->isSelected())
			selectedTile = tile;
		else
			tile->render(tileTrans);
	}

	Renderer::popClipRect();

	// Render the selected image on top of the others
	if (selectedTile != NULL)
		selectedTile->render(tileTrans);

	listRenderTitleOverlay(trans);

	GuiComponent::renderChildren(trans);
}

template<typename T>
void ImageGridComponent<T>::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	// Apply theme to GuiComponent but not size property, which will be applied at the end of this function
	GuiComponent::applyTheme(theme, view, element, properties ^ ThemeFlags::SIZE);

	// Keep the theme pointer to apply it on the tiles later on
	mTheme = theme;

	Vector2f screen = Vector2f((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "imagegrid");
	if (elem)
	{
		if (elem->has("margin"))
			mMargin = elem->get<Vector2f>("margin") * screen;

		if (elem->has("padding"))
			mPadding = elem->get<Vector4f>("padding") * Vector4f(screen.x(), screen.y(), screen.x(), screen.y());

		if (elem->has("autoLayout"))
			mAutoLayout = elem->get<Vector2f>("autoLayout");

		if (elem->has("autoLayoutSelectedZoom"))
			mAutoLayoutZoom = elem->get<float>("autoLayoutSelectedZoom");

		if (elem->has("imageSource"))
		{
			auto direction = elem->get<std::string>("imageSource");
			if (direction == "image")
				mImageSource = IMAGE;
			else if (direction == "marquee")
				mImageSource = MARQUEE;
			else
				mImageSource = THUMBNAIL;
		}
		else
			mImageSource = THUMBNAIL;

		if (elem->has("scrollDirection"))
			mScrollDirection = (ScrollDirection)(elem->get<std::string>("scrollDirection") == "horizontal");

		if (elem->has("centerSelection"))
		{
			mCenterSelection = (elem->get<bool>("centerSelection"));

			if (elem->has("scrollLoop"))
				mScrollLoop = (elem->get<bool>("scrollLoop"));
		}

		if (elem->has("animate"))
			mAnimate = (elem->get<bool>("animate"));
		else
			mAnimate = true;

		if (elem->has("gameImage"))
		{
			std::string path = elem->get<std::string>("gameImage");

			if (!ResourceManager::getInstance()->fileExists(path))
				LOG(LogWarning) << "Could not replace default game image, check path: " << path;
			else
			{
				std::string oldDefaultGameTexture = mDefaultGameTexture;
				mDefaultGameTexture = path;

				// mEntries are already loaded at this point,
				// so we need to update them with new game image texture
				for (auto it = mEntries.begin(); it != mEntries.end(); it++)
				{
					if ((*it).data.texturePath == oldDefaultGameTexture)
						(*it).data.texturePath = mDefaultGameTexture;
				}
			}
		}

		if (elem->has("folderImage"))
		{
			std::string path = elem->get<std::string>("folderImage");

			if (!ResourceManager::getInstance()->fileExists(path))
				LOG(LogWarning) << "Could not replace default folder image, check path: " << path;
			else
			{
				std::string oldDefaultFolderTexture = mDefaultFolderTexture;
				mDefaultFolderTexture = path;

				// mEntries are already loaded at this point,
				// so we need to update them with new folder image texture
				for (auto it = mEntries.begin(); it != mEntries.end(); it++)
				{
					if ((*it).data.texturePath == oldDefaultFolderTexture)
						(*it).data.texturePath = mDefaultFolderTexture;
				}
			}
		}
	}

	// We still need to manually get the grid tile size here,
	// so we can recalculate the new grid dimension, and THEN (re)build the tiles
	elem = theme->getElement(view, "default", "gridtile");

	mTileSize = elem && elem->has("size") ?
				elem->get<Vector2f>("size") * screen :
				GridTileComponent::getDefaultTileSize();

	// Apply size property, will trigger a call to onSizeChanged() which will build the tiles
	GuiComponent::applyTheme(theme, view, element, ThemeFlags::SIZE);

	// Trigger the call manually if the theme have no "imagegrid" element
	if (!elem)
		buildTiles();
}

template<typename T>
void ImageGridComponent<T>::onSizeChanged()
{
	buildTiles();
	updateTiles();
}

template<typename T>
void ImageGridComponent<T>::onCursorChanged(const CursorState& state)
{
	if (mLastCursor == mCursor)
	{
		if (state == CURSOR_STOPPED && mCursorChangedCallback)
			mCursorChangedCallback(state);

		return;
	}

	bool direction = mCursor >= mLastCursor;
	int diff = direction ? mCursor - mLastCursor : mLastCursor - mCursor;
	if (isScrollLoop() && diff == mEntries.size() - 1)
	{
		direction = !direction;
	}

	int oldStart = mStartPosition;

	int dimScrollable = (isVertical() ? mGridDimension.y() : mGridDimension.x()) - 2 * EXTRAITEMS;
	int dimOpposite = isVertical() ? mGridDimension.x() : mGridDimension.y();

	int centralCol = (int)(dimScrollable - 0.5) / 2;
	int maxCentralCol = dimScrollable / 2;

	int oldCol = (mLastCursor / dimOpposite);
	int col = (mCursor / dimOpposite);

	int lastCol = (((int)mEntries.size() - 1) / dimOpposite);

	int lastScroll = std::max(0, (lastCol + 1 - dimScrollable));

	float startPos = 0;
	float endPos = 1;

	if (((GuiComponent*)this)->isAnimationPlaying(2))
	{
		startPos = 0;
		((GuiComponent*)this)->cancelAnimation(2);
		updateTiles(false, false);
	}

	if (mAnimate) {

		std::shared_ptr<GridTileComponent> oldTile = nullptr;
		std::shared_ptr<GridTileComponent> newTile = nullptr;

		int oldIdx = mLastCursor - mStartPosition + (dimOpposite * EXTRAITEMS);
		if (oldIdx >= 0 && oldIdx < mTiles.size())
			oldTile = mTiles[oldIdx];

		int newIdx = mCursor - mStartPosition + (dimOpposite * EXTRAITEMS);
		if (isScrollLoop()) {
			if (newIdx < 0)
				newIdx += (int)mEntries.size();
			else if (newIdx >= mTiles.size())
				newIdx -= (int)mEntries.size();
		}

		if (newIdx >= 0 && newIdx < mTiles.size())
			newTile = mTiles[newIdx];

		for (auto it = mTiles.begin(); it != mTiles.end(); it++) {
			if ((*it)->isSelected() && *it != oldTile && *it != newTile) {
				startPos = 0;
				(*it)->setSelected(false, false, nullptr);
			}
		}

		Vector3f oldPos = Vector3f::Zero();

		if (oldTile != nullptr && oldTile != newTile) {
			oldPos = oldTile->getBackgroundPosition();
			oldTile->setSelected(false, true, nullptr, true);
		}

		if (newTile != nullptr)
			newTile->setSelected(true, true, oldPos == Vector3f::Zero() ? nullptr : &oldPos, true);
	}

	int firstVisibleCol = mStartPosition / dimOpposite;

	if ((col < centralCol || (col == 0 && col == centralCol)) && !mCenterSelection)
		mStartPosition = 0;
	else if ((col - centralCol) > lastScroll && !mCenterSelection && !isScrollLoop())
		mStartPosition = lastScroll * dimOpposite;
	else if ((maxCentralCol != centralCol && col == firstVisibleCol + maxCentralCol) || col == firstVisibleCol + centralCol)
	{
		if (col == firstVisibleCol + maxCentralCol)
			mStartPosition = (col - maxCentralCol) * dimOpposite;
		else
			mStartPosition = (col - centralCol) * dimOpposite;
	}
	else
	{
		if (oldCol == firstVisibleCol + maxCentralCol)
			mStartPosition = (col - maxCentralCol) * dimOpposite;
		else
			mStartPosition = (col - centralCol) * dimOpposite;
	}

	auto lastCursor = mLastCursor;
	mLastCursor = mCursor;

	mCameraDirection = direction ? -1.0f : 1.0f;
	mCamera = 0;

	if (lastCursor < 0 || !mAnimate)
	{
		updateTiles(mAnimate && (lastCursor >= 0 || isScrollLoop()));

		if (mCursorChangedCallback)
			mCursorChangedCallback(state);

		return;
	}

	if (mCursorChangedCallback)
		mCursorChangedCallback(state);

	bool moveCamera = (oldStart != mStartPosition);

	auto func = [this, startPos, endPos, moveCamera](float t)
	{
		if (!moveCamera)
			return;

		t -= 1; // cubic ease out
		float pct = Math::lerp(0, 1, t*t*t + 1);
		t = startPos * (1.0f - pct) + endPos * pct;

		mCamera = t;
	};

	((GuiComponent*)this)->setAnimation(new LambdaAnimation(func, 250), 0, [this] {
		mCamera = 0;
		updateTiles(false);
	}, false, 2);
}


// Create and position tiles (mTiles)
template<typename T>
void ImageGridComponent<T>::buildTiles()
{
	mStartPosition = 0;
	mTiles.clear();

	calcGridDimension();

	if (mCenterSelection)
	{
		int dimScrollable = (isVertical() ? mGridDimension.y() : mGridDimension.x()) - 2 * EXTRAITEMS;
		mStartPosition -= (int) Math::floorf(dimScrollable / 2.0f);
	}

	Vector2f tileDistance = mTileSize + mMargin;

	if (mAutoLayout.x() != 0 && mAutoLayout.y() != 0)
	{
		auto x = (mSize.x() - (mMargin.x() * (mAutoLayout.x() - 1)) - mPadding.x() - mPadding.z()) / (int) mAutoLayout.x();
		auto y = (mSize.y() - (mMargin.y() * (mAutoLayout.y() - 1)) - mPadding.y() - mPadding.w()) / (int) mAutoLayout.y();

		mTileSize = Vector2f(x, y);
		tileDistance = mTileSize + mMargin;
	}

	bool vert = isVertical();

	Vector2f startPosition = mTileSize / 2;

	startPosition += mPadding.v2();

	int X, Y;

	// Layout tile size and position
	for (int y = 0; y < (vert ? mGridDimension.y() : mGridDimension.x()); y++)
	{
		for (int x = 0; x < (vert ? mGridDimension.x() : mGridDimension.y()); x++)
		{
			// Create tiles
			auto tile = std::make_shared<GridTileComponent>(mWindow);

			// In Vertical mod, tiles are ordered from left to right, then from top to bottom
			// In Horizontal mod, tiles are ordered from top to bottom, then from left to right
			X = vert ? x : y - EXTRAITEMS;
			Y = vert ? y - EXTRAITEMS : x;

			tile->setPosition(X * tileDistance.x() + startPosition.x(), Y * tileDistance.y() + startPosition.y());
			tile->setOrigin(0.5f, 0.5f);
			tile->setImage("");

			if (mTheme)
				tile->applyTheme(mTheme, "grid", "gridtile", ThemeFlags::ALL);

			if (mAutoLayout.x() != 0 && mAutoLayout.y() != 0)
				tile->forceSize(mTileSize, mAutoLayoutZoom);

			mTiles.push_back(tile);
		}
	}
}

template<typename T>
void ImageGridComponent<T>::updateTiles(bool allowAnimation, bool updateSelectedState)
{
	if (!mTiles.size())
		return;

	// Stop updating the tiles at highest scroll speed
	if (mScrollTier == 3)
	{
		for (int ti = 0; ti < (int)mTiles.size(); ti++)
		{
			std::shared_ptr<GridTileComponent> tile = mTiles.at(ti);

			tile->setSelected(false);
			tile->setImage(mDefaultGameTexture);
			tile->setVisible(false);
		}
		return;
	}

	// Temporary store previous textures so they can't be unloaded
	std::vector<std::shared_ptr<TextureResource>> previousTextures;
	for (int ti = 0; ti < (int)mTiles.size(); ti++)
	{
		std::shared_ptr<GridTileComponent> tile = mTiles.at(ti);
		previousTextures.push_back(tile->getTexture());
	}

	// Update the tiles
	int firstImg = mStartPosition - EXTRAITEMS * (isVertical() ? mGridDimension.x() : mGridDimension.y());
	for (int ti = 0; ti < (int)mTiles.size(); ti++)
		updateTileAtPos(ti, firstImg + ti, allowAnimation, updateSelectedState);

	if (updateSelectedState)
		mLastCursor = mCursor;

	mLastCursor = mCursor;
}

template<typename T>
void ImageGridComponent<T>::updateTileAtPos(int tilePos, int imgPos, bool allowAnimation, bool updateSelectedState)
{
	std::shared_ptr<GridTileComponent> tile = mTiles.at(tilePos);

	if(isScrollLoop())
	{
		if (imgPos < 0)
			imgPos += (int)mEntries.size();
		else if (imgPos >= size())
			imgPos -= (int)mEntries.size();
	}

	// If we have more tiles than we have to display images on screen, hide them
	if(imgPos < 0 || imgPos >= size() || tilePos < 0 || tilePos >= (int) mTiles.size()) // Same for tiles out of the buffer
	{
		if (updateSelectedState)
			tile->setSelected(false, allowAnimation);

		tile->reset();
		tile->setVisible(false);
	}
	else
	{
		tile->setVisible(true);

		std::string imagePath = mEntries.at(imgPos).data.texturePath;

		if (ResourceManager::getInstance()->fileExists(imagePath))
			tile->setImage(imagePath);
		else if (mEntries.at(imgPos).object->getType() == 2)
			tile->setImage(mDefaultFolderTexture);
		else
			tile->setImage(mDefaultGameTexture);

		if (updateSelectedState)
		{
			if (imgPos == mCursor && mCursor != mLastCursor)
			{
				int dif = mCursor - tilePos;
				int idx = mLastCursor - dif;

				if (idx < 0 || idx >= mTiles.size())
					idx = 0;

				Vector3f pos = mTiles.at(idx)->getBackgroundPosition();
				tile->setSelected(true, allowAnimation, &pos);
			}
			else
				tile->setSelected(imgPos == mCursor, allowAnimation);
		}

	}
}

// Calculate how much tiles of size mTileSize we can fit in a grid of size mSize using a margin of size mMargin
template<typename T>
void ImageGridComponent<T>::calcGridDimension()
{
	// GRID_SIZE = COLUMNS * TILE_SIZE + (COLUMNS - 1) * MARGIN
	// <=> COLUMNS = (GRID_SIZE + MARGIN) / (TILE_SIZE + MARGIN)
	Vector2f gridDimension = (mSize + mMargin) / (mTileSize + mMargin);

	if (mAutoLayout.x() != 0 && mAutoLayout.y() != 0)
		gridDimension = mAutoLayout;

	mLastRowPartial = Math::floorf(gridDimension.y()) != gridDimension.y();

	// Ceil y dim so we can display partial last row
	mGridDimension = Vector2i(gridDimension.x(), Math::ceilf(gridDimension.y()));

	// Grid dimension validation
	if (mGridDimension.x() < 1)
		LOG(LogError) << "Theme defined grid X dimension below 1";
	if (mGridDimension.y() < 1)
		LOG(LogError) << "Theme defined grid Y dimension below 1";

	// Add extra tiles to both sides : Add EXTRAITEMS before, EXTRAITEMS after
	if (isVertical())
		mGridDimension.y() += 2 * EXTRAITEMS;
	else
		mGridDimension.x() += 2 * EXTRAITEMS;
}

template<typename T>
bool ImageGridComponent<T>::isScrollLoop() {
	if (!mScrollLoop)
		return false;
	if (isVertical())
		return (mGridDimension.x() * (mGridDimension.y() - 2 * EXTRAITEMS)) <= mEntries.size();
	return (mGridDimension.y() * (mGridDimension.x() - 2 * EXTRAITEMS)) <= mEntries.size();
};

#endif // ES_CORE_COMPONENTS_IMAGE_GRID_COMPONENT_H
