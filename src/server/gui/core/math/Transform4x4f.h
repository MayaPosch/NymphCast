#pragma once
#ifndef ES_CORE_MATH_TRANSFORM4X4F_H
#define ES_CORE_MATH_TRANSFORM4X4F_H

#include "math/Vector4f.h"
#include "math/Vector3f.h"

class Transform4x4f
{
public:

	Transform4x4f()                                                                                                                            { }
	Transform4x4f(const Vector4f& _r0, const Vector4f& _r1, const Vector4f& _r2, const Vector4f& _r3) : mR0(_r0), mR1(_r1), mR2(_r2), mR3(_r3) { }

	const Transform4x4f operator* (const Transform4x4f& _other) const;
	const Vector3f      operator* (const Vector3f& _other) const;
	Transform4x4f&      operator*=(const Transform4x4f& _other) { *this = *this * _other; return *this; }

	inline       Vector4f& r0()       { return mR0; }
	inline       Vector4f& r1()       { return mR1; }
	inline       Vector4f& r2()       { return mR2; }
	inline       Vector4f& r3()       { return mR3; }
	inline const Vector4f& r0() const { return mR0; }
	inline const Vector4f& r1() const { return mR1; }
	inline const Vector4f& r2() const { return mR2; }
	inline const Vector4f& r3() const { return mR3; }

	Transform4x4f& orthoProjection(float _left, float _right, float _bottom, float _top, float _near, float _far);
	Transform4x4f& invert         (const Transform4x4f& _other);
	Transform4x4f& scale          (const Vector3f& _scale);
	Transform4x4f& rotate         (const float _angle, const Vector3f& _axis);
	Transform4x4f& rotateX        (const float _angle);
	Transform4x4f& rotateY        (const float _angle);
	Transform4x4f& rotateZ        (const float _angle);
	Transform4x4f& translate      (const Vector3f& _translation);
	Transform4x4f& round          ();

	inline       Vector3f& translation()       { return mR3.v3(); }
	inline const Vector3f& translation() const { return mR3.v3(); }

	static const Transform4x4f Identity() { return { { 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 0, 0, 1, 0 }, { 0, 0, 0, 1 } }; }

protected:

	Vector4f mR0;
	Vector4f mR1;
	Vector4f mR2;
	Vector4f mR3;

}; // Transform4x4f

#endif // ES_CORE_MATH_TRANSFORM4X4F_H
