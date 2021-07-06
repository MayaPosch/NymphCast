#pragma once
#ifndef ES_CORE_MATH_VECTOR2I_H
#define ES_CORE_MATH_VECTOR2I_H

#include <assert.h>

class Vector2i
{
public:

	         Vector2i()                                            { }
	         Vector2i(const int _i)               : mX(_i), mY(_i) { }
	         Vector2i(const int _x, const int _y) : mX(_x), mY(_y) { }

	const bool     operator==(const Vector2i& _other) const { return ((mX == _other.mX) && (mY == _other.mY)); }
	const bool     operator!=(const Vector2i& _other) const { return ((mX != _other.mX) || (mY != _other.mY)); }

	const Vector2i operator+ (const Vector2i& _other) const { return { mX + _other.mX, mY + _other.mY }; }
	const Vector2i operator- (const Vector2i& _other) const { return { mX - _other.mX, mY - _other.mY }; }
	const Vector2i operator* (const Vector2i& _other) const { return { mX * _other.mX, mY * _other.mY }; }
	const Vector2i operator/ (const Vector2i& _other) const { return { mX / _other.mX, mY / _other.mY }; }

	const Vector2i operator+ (const int& _other) const      { return { mX + _other, mY + _other }; }
	const Vector2i operator- (const int& _other) const      { return { mX - _other, mY - _other }; }
	const Vector2i operator* (const int& _other) const      { return { mX * _other, mY * _other }; }
	const Vector2i operator/ (const int& _other) const      { return { mX / _other, mY / _other }; }

	const Vector2i operator- () const                       { return { -mX , -mY }; }

	Vector2i&      operator+=(const Vector2i& _other)       { *this = *this + _other; return *this; }
	Vector2i&      operator-=(const Vector2i& _other)       { *this = *this - _other; return *this; }
	Vector2i&      operator*=(const Vector2i& _other)       { *this = *this * _other; return *this; }
	Vector2i&      operator/=(const Vector2i& _other)       { *this = *this / _other; return *this; }

	Vector2i&      operator+=(const int& _other)            { *this = *this + _other; return *this; }
	Vector2i&      operator-=(const int& _other)            { *this = *this - _other; return *this; }
	Vector2i&      operator*=(const int& _other)            { *this = *this * _other; return *this; }
	Vector2i&      operator/=(const int& _other)            { *this = *this / _other; return *this; }

	      int&     operator[](const int _index)             { assert(_index < 2 && "index out of range"); return (&mX)[_index]; }
	const int&     operator[](const int _index) const       { assert(_index < 2 && "index out of range"); return (&mX)[_index]; }

	inline       int& x()       { return mX; }
	inline       int& y()       { return mY; }
	inline const int& x() const { return mX; }
	inline const int& y() const { return mY; }

	static const Vector2i Zero () { return { 0, 0 }; }
	static const Vector2i UnitX() { return { 1, 0 }; }
	static const Vector2i UnitY() { return { 0, 1 }; }

private:

	int mX;
	int mY;

}; // Vector2i

#endif // ES_CORE_MATH_VECTOR2I_H
