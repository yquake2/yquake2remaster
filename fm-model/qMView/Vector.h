typedef float vec3a_t[3];
typedef double vec3d_t[3];

// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

void DirAndUpFromAngles(vec3a_t angles, vec3a_t direction, vec3a_t up);
void AnglesFromDir(vec3a_t direction, vec3a_t angles);
void AnglesFromDirAndUp(vec3a_t direction, vec3a_t up, vec3a_t angles);

_inline void Vec3SubtractAssign(vec3a_t value, vec3a_t subFrom)
{
	subFrom[0] -= value[0];
	subFrom[1] -= value[1];
	subFrom[2] -= value[2];
}

_inline void Vec3AddAssign(vec3a_t value, vec3a_t addTo)
{
	addTo[0] += value[0];
	addTo[1] += value[1];
	addTo[2] += value[2];
}

_inline float Vec3DotProduct(vec3a_t v1, vec3a_t v2)
{
	return (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2]);
}

_inline float Vec3Normalize(vec3a_t v1)
{
	float mag;
	float imag = 1;

	mag = Vec3DotProduct(v1, v1);

	if(!mag)
	{
		return 0;
	}

	mag = (float)sqrt(mag);

	imag /= mag;

	v1[0] *= imag;
	v1[1] *= imag;
	v1[2] *= imag;

	return mag;
}

_inline double Vec3dDotProduct(vec3d_t v1, vec3d_t v2)
{
	return (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2]);
}

_inline double Vec3dNormalize(vec3d_t v1)
{
	double mag;
	double imag = 1;

	mag = Vec3dDotProduct(v1, v1);

	if(!mag)
	{
		return 0;
	}

	mag = (float)sqrt(mag);

	imag /= mag;

	v1[0] *= imag;
	v1[1] *= imag;
	v1[2] *= imag;

	return mag;
}
