#include "math/Transform4x4f.h"

//////////////////////////////////////////////////////////////////////////

const Transform4x4f Transform4x4f::operator*(const Transform4x4f& _other) const
{
	const float* tm = (float*)this;
	const float* om = (float*)&_other;

	return
	{
		{
			tm[ 0] * om[ 0] + tm[ 4] * om[ 1] + tm[ 8] * om[ 2],
			tm[ 1] * om[ 0] + tm[ 5] * om[ 1] + tm[ 9] * om[ 2],
			tm[ 2] * om[ 0] + tm[ 6] * om[ 1] + tm[10] * om[ 2],
			0
		},
		{
			tm[ 0] * om[ 4] + tm[ 4] * om[ 5] + tm[ 8] * om[ 6],
			tm[ 1] * om[ 4] + tm[ 5] * om[ 5] + tm[ 9] * om[ 6],
			tm[ 2] * om[ 4] + tm[ 6] * om[ 5] + tm[10] * om[ 6],
			0
		},
		{
			tm[ 0] * om[ 8] + tm[ 4] * om[ 9] + tm[ 8] * om[10],
			tm[ 1] * om[ 8] + tm[ 5] * om[ 9] + tm[ 9] * om[10],
			tm[ 2] * om[ 8] + tm[ 6] * om[ 9] + tm[10] * om[10],
			0
		},
		{
			tm[ 0] * om[12] + tm[ 4] * om[13] + tm[ 8] * om[14] + tm[12],
			tm[ 1] * om[12] + tm[ 5] * om[13] + tm[ 9] * om[14] + tm[13],
			tm[ 2] * om[12] + tm[ 6] * om[13] + tm[10] * om[14] + tm[14],
			1
		}
	};

} // operator*

//////////////////////////////////////////////////////////////////////////

const Vector3f Transform4x4f::operator*(const Vector3f& _other) const
{
	const float* tm = (float*)this;
	const float* ov = (float*)&_other;

	return
	{
		tm[ 0] * ov[0] + tm[ 4] * ov[1] + tm[ 8] * ov[2] + tm[12],
		tm[ 1] * ov[0] + tm[ 5] * ov[1] + tm[ 9] * ov[2] + tm[13],
		tm[ 2] * ov[0] + tm[ 6] * ov[1] + tm[10] * ov[2] + tm[14]
	};

} // operator*

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::orthoProjection(float _left, float _right, float _bottom, float _top, float _near, float _far)
{
	float*      tm       = (float*)this;
	const float o[6]     = {  2 / (_right - _left),
	                          2 / (_top   - _bottom),
	                         -2 / (_far   - _near),
	                         -(_right + _left)   / (_right - _left),
	                         -(_top   + _bottom) / (_top   - _bottom),
	                         -(_far   + _near)   / (_far   - _near) };
	const float temp[12] = { tm[ 0] * o[0],
	                         tm[ 1] * o[0],
	                         tm[ 2] * o[0],
	                         tm[ 4] * o[1],
	                         tm[ 5] * o[1],
	                         tm[ 6] * o[1],
	                         tm[ 8] * o[2],
	                         tm[ 9] * o[2],
	                         tm[10] * o[2],
	                         tm[ 0] * o[3] + tm[ 4] * o[4] + tm[ 8] * o[5] + tm[12],
	                         tm[ 1] * o[3] + tm[ 5] * o[4] + tm[ 9] * o[5] + tm[13],
	                         tm[ 2] * o[3] + tm[ 6] * o[4] + tm[10] * o[5] + tm[14] };

	tm[ 0] = temp[ 0];
	tm[ 1] = temp[ 1];
	tm[ 2] = temp[ 2];
	tm[ 4] = temp[ 3];
	tm[ 5] = temp[ 4];
	tm[ 6] = temp[ 5];
	tm[ 8] = temp[ 6];
	tm[ 9] = temp[ 7];
	tm[10] = temp[ 8];
	tm[12] = temp[ 9];
	tm[13] = temp[10];
	tm[14] = temp[11];

	return *this;

} // orthoProjection

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::invert(const Transform4x4f& _other)
{
	float*       tm = (float*)this;
	const float* om = (float*)&_other;

	// Full invert
	// tm[ 0] =  ((om[ 5] * (om[10] * om[15] - om[11] * om[14])) - (om[ 9] * (om[ 6] * om[15] - om[ 7] * om[14])) + (om[13] * (om[ 6] * om[11] - om[ 7] * om[10])));
	// tm[ 1] = -((om[ 1] * (om[10] * om[15] - om[11] * om[14])) - (om[ 9] * (om[ 2] * om[15] - om[ 3] * om[14])) + (om[13] * (om[ 2] * om[11] - om[ 3] * om[10])));
	// tm[ 2] =  ((om[ 1] * (om[ 6] * om[15] - om[ 7] * om[14])) - (om[ 5] * (om[ 2] * om[15] - om[ 3] * om[14])) + (om[13] * (om[ 2] * om[ 7] - om[ 3] * om[ 6])));
	// tm[ 3] = -((om[ 1] * (om[ 6] * om[11] - om[ 7] * om[10])) - (om[ 5] * (om[ 2] * om[11] - om[ 3] * om[10])) + (om[ 9] * (om[ 2] * om[ 7] - om[ 3] * om[ 6])));
	// tm[ 4] = -((om[ 4] * (om[10] * om[15] - om[11] * om[14])) - (om[ 8] * (om[ 6] * om[15] - om[ 7] * om[14])) + (om[12] * (om[ 6] * om[11] - om[ 7] * om[10])));
	// tm[ 5] =  ((om[ 0] * (om[10] * om[15] - om[11] * om[14])) - (om[ 8] * (om[ 2] * om[15] - om[ 3] * om[14])) + (om[12] * (om[ 2] * om[11] - om[ 3] * om[10])));
	// tm[ 6] = -((om[ 0] * (om[ 6] * om[15] - om[ 7] * om[14])) - (om[ 4] * (om[ 2] * om[15] - om[ 3] * om[14])) + (om[12] * (om[ 2] * om[ 7] - om[ 3] * om[ 6])));
	// tm[ 7] =  ((om[ 0] * (om[ 6] * om[11] - om[ 7] * om[10])) - (om[ 4] * (om[ 2] * om[11] - om[ 3] * om[10])) + (om[ 8] * (om[ 2] * om[ 7] - om[ 3] * om[ 6])));
	// tm[ 8] =  ((om[ 4] * (om[ 9] * om[15] - om[11] * om[13])) - (om[ 8] * (om[ 5] * om[15] - om[ 7] * om[13])) + (om[12] * (om[ 5] * om[11] - om[ 7] * om[ 9])));
	// tm[ 9] = -((om[ 0] * (om[ 9] * om[15] - om[11] * om[13])) - (om[ 8] * (om[ 1] * om[15] - om[ 3] * om[13])) + (om[12] * (om[ 1] * om[11] - om[ 3] * om[ 9])));
	// tm[10] =  ((om[ 0] * (om[ 5] * om[15] - om[ 7] * om[13])) - (om[ 4] * (om[ 1] * om[15] - om[ 3] * om[13])) + (om[12] * (om[ 1] * om[ 7] - om[ 3] * om[ 5])));
	// tm[11] = -((om[ 0] * (om[ 5] * om[11] - om[ 7] * om[ 9])) - (om[ 4] * (om[ 1] * om[11] - om[ 3] * om[ 9])) + (om[ 8] * (om[ 1] * om[ 7] - om[ 3] * om[ 5])));
	// tm[12] = -((om[ 4] * (om[ 9] * om[14] - om[10] * om[13])) - (om[ 8] * (om[ 5] * om[14] - om[ 6] * om[13])) + (om[12] * (om[ 5] * om[10] - om[ 6] * om[ 9])));
	// tm[13] =  ((om[ 0] * (om[ 9] * om[14] - om[10] * om[13])) - (om[ 8] * (om[ 1] * om[14] - om[ 2] * om[13])) + (om[12] * (om[ 1] * om[10] - om[ 2] * om[ 9])));
	// tm[14] = -((om[ 0] * (om[ 5] * om[14] - om[ 6] * om[13])) - (om[ 4] * (om[ 1] * om[14] - om[ 2] * om[13])) + (om[12] * (om[ 1] * om[ 6] - om[ 2] * om[ 5])));
	// tm[15] =  ((om[ 0] * (om[ 5] * om[10] - om[ 6] * om[ 9])) - (om[ 4] * (om[ 1] * om[10] - om[ 2] * om[ 9])) + (om[ 8] * (om[ 1] * om[ 6] - om[ 2] * om[ 5])));

	// Optimized invert ( om[3, 7 and 11] is always 0, and om[15] is always 1 )
	tm[ 0] =  ((om[ 5] * om[10]) - (om[ 9] * om[ 6]));
	tm[ 1] = -((om[ 1] * om[10]) - (om[ 9] * om[ 2]));
	tm[ 2] =  ((om[ 1] * om[ 6]) - (om[ 5] * om[ 2]));
	tm[ 3] =  0;
	tm[ 4] = -((om[ 4] * om[10]) - (om[ 8] * om[ 6]));
	tm[ 5] =  ((om[ 0] * om[10]) - (om[ 8] * om[ 2]));
	tm[ 6] = -((om[ 0] * om[ 6]) - (om[ 4] * om[ 2]));
	tm[ 7] =  0;
	tm[ 8] =  ((om[ 4] * om[ 9]) - (om[ 8] * om[ 5]));
	tm[ 9] = -((om[ 0] * om[ 9]) - (om[ 8] * om[ 1]));
	tm[10] =  ((om[ 0] * om[ 5]) - (om[ 4] * om[ 1]));
	tm[11] =  0;
	tm[12] = -((om[ 4] * (om[ 9] * om[14] - om[10] * om[13])) - (om[ 8] * (om[ 5] * om[14] - om[ 6] * om[13])) + (om[12] * (om[ 5] * om[10] - om[ 6] * om[ 9])));
	tm[13] =  ((om[ 0] * (om[ 9] * om[14] - om[10] * om[13])) - (om[ 8] * (om[ 1] * om[14] - om[ 2] * om[13])) + (om[12] * (om[ 1] * om[10] - om[ 2] * om[ 9])));
	tm[14] = -((om[ 0] * (om[ 5] * om[14] - om[ 6] * om[13])) - (om[ 4] * (om[ 1] * om[14] - om[ 2] * om[13])) + (om[12] * (om[ 1] * om[ 6] - om[ 2] * om[ 5])));
	tm[15] =  1;

	float Determinant = om[ 0] * tm[ 0] +
	                    om[ 4] * tm[ 1] +
	                    om[ 8] * tm[ 2] +
	                    om[12] * tm[ 3];

	if(Determinant != 0)
		Determinant = 1 / Determinant;

	tm[ 0] *= Determinant;
	tm[ 1] *= Determinant;
	tm[ 2] *= Determinant;
	tm[ 4] *= Determinant;
	tm[ 5] *= Determinant;
	tm[ 6] *= Determinant;
	tm[ 8] *= Determinant;
	tm[ 9] *= Determinant;
	tm[10] *= Determinant;
	tm[12] *= Determinant;
	tm[13] *= Determinant;
	tm[14] *= Determinant;

	return *this;

} // invert

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::scale(const Vector3f& _scale)
{
	float*       tm = (float*)this;
	const float* sv = (float*)&_scale;

	tm[ 0] *= sv[0];
	tm[ 1] *= sv[0];
	tm[ 2] *= sv[0];
	tm[ 4] *= sv[1];
	tm[ 5] *= sv[1];
	tm[ 6] *= sv[1];
	tm[ 8] *= sv[2];
	tm[ 9] *= sv[2];
	tm[10] *= sv[2];

	return *this;

} // scale

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::rotate(const float _angle, const Vector3f& _axis)
{
	float*       tm      = (float*)this;
	const float* av      = (float*)&_axis;
	const float  s       = Math::sinf(-_angle);
	const float  c       = Math::cosf(-_angle);
	const float  t       = 1 - c;
	const float  x       = av[0];
	const float  y       = av[1];
	const float  z       = av[2];
	const float  tx      = t * x;
	const float  ty      = t * y;
	const float  tz      = t * z;
	const float  sx      = s * x;
	const float  sy      = s * y;
	const float  sz      = s * z;
	const float  r[9]    = { tx * x + c,
	                         tx * y - sz,
	                         tx * z + sy,
	                         ty * x + sz,
	                         ty * y + c,
	                         ty * z - sx,
	                         tz * x - sy,
	                         tz * y + sx,
	                         tz * z + c };
	const float  temp[9] = { tm[ 0] * r[0] + tm[ 4] * r[1] + tm[ 8] * r[2],
	                         tm[ 1] * r[0] + tm[ 5] * r[1] + tm[ 9] * r[2],
	                         tm[ 2] * r[0] + tm[ 6] * r[1] + tm[10] * r[2],
	                         tm[ 0] * r[3] + tm[ 4] * r[4] + tm[ 8] * r[5],
	                         tm[ 1] * r[3] + tm[ 5] * r[4] + tm[ 9] * r[5],
	                         tm[ 2] * r[3] + tm[ 6] * r[4] + tm[ 0] * r[5],
	                         tm[ 0] * r[6] + tm[ 4] * r[7] + tm[ 8] * r[8],
	                         tm[ 1] * r[6] + tm[ 5] * r[7] + tm[ 9] * r[8],
	                         tm[ 2] * r[6] + tm[ 6] * r[7] + tm[10] * r[8] };

	tm[ 0] = temp[0];
	tm[ 1] = temp[1];
	tm[ 2] = temp[2];
	tm[ 4] = temp[3];
	tm[ 5] = temp[4];
	tm[ 6] = temp[5];
	tm[ 8] = temp[6];
	tm[ 9] = temp[7];
	tm[10] = temp[8];

	return *this;

}; // rotate

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::rotateX(const float _angle)
{
	float*      tm      = (float*)this;
	const float s       = Math::sinf(-_angle);
	const float c       = Math::cosf(-_angle);
	const float temp[6] = { tm[ 4] * c + tm[ 8] * -s,
	                        tm[ 5] * c + tm[ 9] * -c,
	                        tm[ 6] * c + tm[10] * -s,
	                        tm[ 4] * s + tm[ 8] *  c,
	                        tm[ 5] * s + tm[ 9] *  c,
	                        tm[ 6] * s + tm[10] *  c };

	tm[ 4] = temp[0];
	tm[ 5] = temp[1];
	tm[ 6] = temp[2];
	tm[ 8] = temp[3];
	tm[ 9] = temp[4];
	tm[10] = temp[5];

	return *this;

}; // rotateX

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::rotateY(const float _angle)
{
	float*      tm      = (float*)this;
	const float s       = Math::sinf(-_angle);
	const float c       = Math::cosf(-_angle);
	const float temp[6] = { tm[ 0] *  c + tm[ 8] * s,
	                        tm[ 1] *  c + tm[ 9] * s,
	                        tm[ 2] *  c + tm[10] * s,
	                        tm[ 0] * -s + tm[ 8] * c,
	                        tm[ 1] * -s + tm[ 9] * c,
	                        tm[ 2] * -s + tm[10] * c };

	tm[ 0] = temp[0];
	tm[ 1] = temp[1];
	tm[ 2] = temp[2];
	tm[ 8] = temp[3];
	tm[ 9] = temp[4];
	tm[10] = temp[5];

	return *this;

}; // rotateY

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::rotateZ(const float _angle)
{
	float*      tm      = (float*)this;
	const float s       = Math::sinf(-_angle);
	const float c       = Math::cosf(-_angle);
	const float temp[6] = { tm[ 0] * c + tm[ 4] * -s,
	                        tm[ 1] * c + tm[ 5] * -s,
	                        tm[ 2] * c + tm[ 6] * -s,
	                        tm[ 0] * s + tm[ 4] *  c,
	                        tm[ 1] * s + tm[ 5] *  c,
	                        tm[ 2] * s + tm[ 6] *  c };

	tm[ 0] = temp[0];
	tm[ 1] = temp[1];
	tm[ 2] = temp[2];
	tm[ 4] = temp[3];
	tm[ 5] = temp[4];
	tm[ 6] = temp[5];

	return *this;

}; // rotateZ

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::translate(const Vector3f& _translation)
{
	float*       tm = (float*)this;
	const float* tv = (float*)&_translation;

	tm[12] += tm[ 0] * tv[0] + tm[ 4] * tv[1] + tm[ 8] * tv[2];
	tm[13] += tm[ 1] * tv[0] + tm[ 5] * tv[1] + tm[ 9] * tv[2];
	tm[14] += tm[ 2] * tv[0] + tm[ 6] * tv[1] + tm[10] * tv[2];

	return *this;

} // translate

//////////////////////////////////////////////////////////////////////////

Transform4x4f& Transform4x4f::round()
{
	float* tm = (float*)this;

	tm[12] = Math::round(tm[12]);
	tm[13] = Math::round(tm[13]);
	tm[14] = Math::round(tm[14]);

	return *this;

} // round
