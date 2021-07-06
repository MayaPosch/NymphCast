#pragma once
#ifndef ES_CORE_MATH_VECTOR2F_H
#define ES_CORE_MATH_VECTOR2F_H

#include "math/Misc.h"
#include <assert.h>

class Vector3f;
class Vector4f;

class Vector2f
{
public:

	         Vector2f()                                                                                { }
	         Vector2f(const float _f)                 : mX(_f),                 mY(_f)                 { }
	         Vector2f(const float _x, const float _y) : mX(_x),                 mY(_y)                 { }
	explicit Vector2f(const Vector3f& _v)             : mX(((Vector2f&)_v).mX), mY(((Vector2f&)_v).mY) { }
	explicit Vector2f(const Vector4f& _v)             : mX(((Vector2f&)_v).mX), mY(((Vector2f&)_v).mY) { }

	const bool     operator==(const Vector2f& _other) const { return ((mX == _other.mX) && (mY == _other.mY)); }
	const bool     operator!=(const Vector2f& _other) const { return ((mX != _other.mX) || (mY != _other.mY)); }

	const Vector2f operator+ (const Vector2f& _other) const { return { mX + _other.mX, mY + _other.mY }; }
	const Vector2f operator- (const Vector2f& _other) const { return { mX - _other.mX, mY - _other.mY }; }
	const Vector2f operator* (const Vector2f& _other) const { return { mX * _other.mX, mY * _other.mY }; }
	const Vector2f operator/ (const Vector2f& _other) const { return { mX / _other.mX, mY / _other.mY }; }

	const Vector2f operator+ (const float& _other) const    { return { mX + _other, mY + _other }; }
	const Vector2f operator- (const float& _other) const    { return { mX - _other, mY - _other }; }
	const Vector2f operator* (const float& _other) const    { return { mX * _other, mY * _other }; }
	const Vector2f operator/ (const float& _other) const    { return { mX / _other, mY / _other }; }

	const Vector2f operator- () const                       { return { -mX , -mY }; }

	Vector2f&      operator+=(const Vector2f& _other)       { *this = *this + _other; return *this; }
	Vector2f&      operator-=(const Vector2f& _other)       { *this = *this - _other; return *this; }
	Vector2f&      operator*=(const Vector2f& _other)       { *this = *this * _other; return *this; }
	Vector2f&      operator/=(const Vector2f& _other)       { *this = *this / _other; return *this; }

	Vector2f&      operator+=(const float& _other)          { *this = *this + _other; return *this; }
	Vector2f&      operator-=(const float& _other)          { *this = *this - _other; return *this; }
	Vector2f&      operator*=(const float& _other)          { *this = *this * _other; return *this; }
	Vector2f&      operator/=(const float& _other)          { *this = *this / _other; return *this; }

	      float&   operator[](const int _index)             { assert(_index < 2 && "index out of range"); return (&mX)[_index]; }
	const float&   operator[](const int _index) const       { assert(_index < 2 && "index out of range"); return (&mX)[_index]; }

	inline       float& x()       { return mX; }
	inline       float& y()       { return mY; }
	inline const float& x() const { return mX; }
	inline const float& y() const { return mY; }

	Vector2f& round();
	Vector2f& lerp (const Vector2f& _start, const Vector2f& _end, const float _fraction);

	static const Vector2f Zero () { return { 0, 0 }; }
	static const Vector2f UnitX() { return { 1, 0 }; }
	static const Vector2f UnitY() { return { 0, 1 }; }

private:

	float mX;
	float mY;

}; // Vector2f

#endif // ES_CORE_MATH_VECTOR2F_H
