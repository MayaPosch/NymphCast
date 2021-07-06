#include "components/NinePatchComponent.h"

#include "resources/TextureResource.h"
#include "Log.h"
#include "ThemeData.h"

NinePatchComponent::NinePatchComponent(Window* window, const std::string& path, unsigned int edgeColor, unsigned int centerColor) : GuiComponent(window),
	mCornerSize(16, 16),
	mEdgeColor(edgeColor), mCenterColor(centerColor),
	mPath(path),
	mVertices(NULL)
{
	if(!mPath.empty())
		buildVertices();
}

NinePatchComponent::~NinePatchComponent()
{
	if (mVertices != NULL)
		delete[] mVertices;
}

void NinePatchComponent::updateColors()
{
	const unsigned int edgeColor   = Renderer::convertColor(mEdgeColor);
	const unsigned int centerColor = Renderer::convertColor(mCenterColor);

	for(int i = 0; i < 6*9; ++i)
		mVertices[i].col = edgeColor;

	for(int i = 6*4; i < 6; ++i)
		mVertices[(6*4)+i].col = centerColor;
}

void NinePatchComponent::buildVertices()
{
	if(mVertices != NULL)
		delete[] mVertices;

	mTexture = TextureResource::get(mPath);

	if(mTexture->getSize() == Vector2i::Zero())
	{
		mVertices = NULL;
		LOG(LogWarning) << "NinePatchComponent missing texture!";
		return;
	}

	mVertices = new Renderer::Vertex[6 * 9];

	const Vector2f texSize = Vector2f((float)mTexture->getSize().x(), (float)mTexture->getSize().y());

	const float imgSizeX[3] = { mCornerSize.x(), mSize.x() - mCornerSize.x() * 2, mCornerSize.x()};
	const float imgSizeY[3] = { mCornerSize.y(), mSize.y() - mCornerSize.y() * 2, mCornerSize.y()};
	const float imgPosX[3]  = { 0, imgSizeX[0], imgSizeX[0] + imgSizeX[1]};
	const float imgPosY[3]  = { 0, imgSizeY[0], imgSizeY[0] + imgSizeY[1]};

	//the "1 +" in posY and "-" in sizeY is to deal with texture coordinates having a bottom left corner origin vs. verticies having a top left origin
	const float texSizeX[3] = {  mCornerSize.x() / texSize.x(),  (texSize.x() - mCornerSize.x() * 2) / texSize.x(),  mCornerSize.x() / texSize.x() };
	const float texSizeY[3] = { -mCornerSize.y() / texSize.y(), -(texSize.y() - mCornerSize.y() * 2) / texSize.y(), -mCornerSize.y() / texSize.y() };
	const float texPosX[3]  = {  0,     texSizeX[0],     texSizeX[0] + texSizeX[1] };
	const float texPosY[3]  = {  1, 1 + texSizeY[0], 1 + texSizeY[0] + texSizeY[1] };

	int v = 0;
	for(int slice = 0; slice < 9; slice++)
	{
		const int      sliceX  = slice % 3;
		const int      sliceY  = slice / 3;
		const Vector2f imgPos  = Vector2f(imgPosX[sliceX], imgPosY[sliceY]);
		const Vector2f imgSize = Vector2f(imgSizeX[sliceX], imgSizeY[sliceY]);
		const Vector2f texPos  = Vector2f(texPosX[sliceX], texPosY[sliceY]);
		const Vector2f texSize = Vector2f(texSizeX[sliceX], texSizeY[sliceY]);

		mVertices[v + 1] = { { imgPos.x()              , imgPos.y()               }, { texPos.x(),               texPos.y()               }, 0 };
		mVertices[v + 2] = { { imgPos.x()              , imgPos.y() + imgSize.y() }, { texPos.x(),               texPos.y() + texSize.y() }, 0 };
		mVertices[v + 3] = { { imgPos.x() + imgSize.x(), imgPos.y()               }, { texPos.x() + texSize.x(), texPos.y()               }, 0 };
		mVertices[v + 4] = { { imgPos.x() + imgSize.x(), imgPos.y() + imgSize.y() }, { texPos.x() + texSize.x(), texPos.y() + texSize.y() }, 0 };

		// round vertices
		for(int i = 1; i < 5; ++i)
			mVertices[v + i].pos.round();

		// make duplicates of first and last vertex so this can be rendered as a triangle strip
		mVertices[v + 0] = mVertices[v + 1];
		mVertices[v + 5] = mVertices[v + 4];

		v += 6;
	}

	updateColors();
}

void NinePatchComponent::render(const Transform4x4f& parentTrans)
{
	if (!isVisible())
		return;

	Transform4x4f trans = parentTrans * getTransform();

	if(mTexture && mVertices != NULL)
	{
		Renderer::setMatrix(trans);

		mTexture->bind();
		Renderer::drawTriangleStrips(&mVertices[0], 6*9);
	}

	renderChildren(trans);
}

void NinePatchComponent::onSizeChanged()
{
	buildVertices();
}

const Vector2f& NinePatchComponent::getCornerSize() const
{
	return mCornerSize;
}

void NinePatchComponent::setCornerSize(int sizeX, int sizeY)
{
	mCornerSize = Vector2f(sizeX, sizeY);
	buildVertices();
}

void NinePatchComponent::fitTo(Vector2f size, Vector3f position, Vector2f padding)
{
	size += padding;
	position[0] -= padding.x() / 2;
	position[1] -= padding.y() / 2;

	setSize(size + mCornerSize * 2);
	setPosition(position.x() + Math::lerp(-mCornerSize.x(), mCornerSize.x(), mOrigin.x()),
				position.y() + Math::lerp(-mCornerSize.y(), mCornerSize.y(), mOrigin.y()));
}

void NinePatchComponent::setImagePath(const std::string& path)
{
	mPath = path;
	buildVertices();
}

void NinePatchComponent::setEdgeColor(unsigned int edgeColor)
{
	mEdgeColor = edgeColor;
	updateColors();
}

void NinePatchComponent::setCenterColor(unsigned int centerColor)
{
	mCenterColor = centerColor;
	updateColors();
}

void NinePatchComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& element, unsigned int properties)
{
	GuiComponent::applyTheme(theme, view, element, properties);

	using namespace ThemeFlags;

	const ThemeData::ThemeElement* elem = theme->getElement(view, element, "ninepatch");
	if(!elem)
		return;

	if(properties & PATH && elem->has("path"))
		setImagePath(elem->get<std::string>("path"));
}
