#include "math/Vector4f.h"

//////////////////////////////////////////////////////////////////////////

Vector4f& Vector4f::round()
{
	mX = Math::round(mX);
	mY = Math::round(mY);
	mZ = Math::round(mZ);
	mW = Math::round(mW);

	return *this;

} // round

//////////////////////////////////////////////////////////////////////////

Vector4f& Vector4f::lerp(const Vector4f& _start, const Vector4f& _end, const float _fraction)
{
	mX = Math::lerp(_start.x(), _end.x(), _fraction);
	mY = Math::lerp(_start.y(), _end.y(), _fraction);
	mZ = Math::lerp(_start.z(), _end.z(), _fraction);
	mW = Math::lerp(_start.w(), _end.w(), _fraction);

	return *this;

} // lerp
