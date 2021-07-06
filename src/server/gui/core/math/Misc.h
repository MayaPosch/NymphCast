#pragma once
#ifndef ES_CORE_MATH_MISC_H
#define ES_CORE_MATH_MISC_H

#define	ES_PI (3.1415926535897932384626433832795028841971693993751058209749445923)
#define	ES_RAD_TO_DEG(_x) ((_x) * (180.0 / ES_PI))
#define	ES_DEG_TO_RAD(_x) ((_x) * (ES_PI / 180.0))

namespace Math
{
	// added here to avoid including math.h whenever these are used
	float cosf        (const float _num);
	float sinf        (const float _num);
	float floorf      (const float _num);
	float ceilf       (const float _num);

	int   min         (const int _num1, const int _num2);
	int   max         (const int _num1, const int _num2);
	int   clamp       (const int _num, const int _min, const int _max);
	float min         (const float _num1, const float _num2);
	float max         (const float _num1, const float _num2);
	float clamp       (const float _num, const float _min, const float _max);
	float round       (const float _num);
	float lerp        (const float _start, const float _end, const float _fraction);
	float smoothStep  (const float _left, const float _right, const float _x);
	float smootherStep(const float _left, const float _right, const float _x);

	namespace Scroll
	{
		float bounce(const float _delayTime, const float _scrollTime, const float _currentTime, const float _scrollLength);
		float loop  (const float _delayTime, const float _scrollTime, const float _currentTime, const float _scrollLength);

	} // Scroll::

} // Math::

#endif // ES_CORE_MATH_MISC_H
