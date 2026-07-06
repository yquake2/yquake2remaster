#include "stdafx.h"
#include "Matrix.h"
#include "Vector.h"
#include "Angles.h"

#pragma warning(disable : 4244)		// truncation from double to float
#pragma warning(disable : 4056)		// overflow in floating-point constant arithmetic
#pragma warning (disable : 4305)

void CMatrix::Matrix3MultByMartrix3(matrix3_t A, matrix3_t B, matrix3_t C)
{
	C[0][0] = B[0][0] * A[0][0] + B[0][1] * A[1][0] +
				B[0][2] * A[2][0];
	C[0][1] = B[0][0] * A[0][1] + B[0][1] * A[1][1] +
				B[0][2] * A[2][1];
	C[0][2] = B[0][0] * A[0][2] + B[0][1] * A[1][2] +
				B[0][2] * A[2][2];
	C[1][0] = B[1][0] * A[0][0] + B[1][1] * A[1][0] +
				B[1][2] * A[2][0];
	C[1][1] = B[1][0] * A[0][1] + B[1][1] * A[1][1] +
				B[1][2] * A[2][1];
	C[1][2] = B[1][0] * A[0][2] + B[1][1] * A[1][2] +
				B[1][2] * A[2][2];
	C[2][0] = B[2][0] * A[0][0] + B[2][1] * A[1][0] +
				B[2][2] * A[2][0];
	C[2][1] = B[2][0] * A[0][1] + B[2][1] * A[1][1] +
				B[2][2] * A[2][1];
	C[2][2] = B[2][0] * A[0][2] + B[2][1] * A[1][2] +
				B[2][2] * A[2][2];
}

void CMatrix::Matrix3dMultByMartrix3d(matrix3d_t A, matrix3d_t B, matrix3d_t C)
{
	C[0][0] = B[0][0] * A[0][0] + B[0][1] * A[1][0] +
				B[0][2] * A[2][0];
	C[0][1] = B[0][0] * A[0][1] + B[0][1] * A[1][1] +
				B[0][2] * A[2][1];
	C[0][2] = B[0][0] * A[0][2] + B[0][1] * A[1][2] +
				B[0][2] * A[2][2];
	C[1][0] = B[1][0] * A[0][0] + B[1][1] * A[1][0] +
				B[1][2] * A[2][0];
	C[1][1] = B[1][0] * A[0][1] + B[1][1] * A[1][1] +
				B[1][2] * A[2][1];
	C[1][2] = B[1][0] * A[0][2] + B[1][1] * A[1][2] +
				B[1][2] * A[2][2];
	C[2][0] = B[2][0] * A[0][0] + B[2][1] * A[1][0] +
				B[2][2] * A[2][0];
	C[2][1] = B[2][0] * A[0][1] + B[2][1] * A[1][1] +
				B[2][2] * A[2][1];
	C[2][2] = B[2][0] * A[0][2] + B[2][1] * A[1][2] +
				B[2][2] * A[2][2];
}

void CMatrix::Matrix3MultByVec3(matrix3_t A, vec3a_t B, vec3a_t C)
{
	C[0] = B[0] * A[0][0] + B[1] * A[1][0] +
				B[2] * A[2][0];
	C[1] = B[0] * A[0][1] + B[1] * A[1][1] +
				B[2] * A[2][1];
	C[2] = B[0] * A[0][2] + B[1] * A[1][2] +
				B[2] * A[2][2];
}

void CMatrix::Matrix3dMultByVec3(matrix3d_t A, vec3a_t B, vec3a_t *C)
{
	C[0][0] = B[0] * A[0][0] + B[1] * A[1][0] +
				B[2] * A[2][0];
	C[0][1] = B[0] * A[0][1] + B[1] * A[1][1] +
				B[2] * A[2][1];
	C[0][2] = B[0] * A[0][2] + B[1] * A[1][2] +
				B[2] * A[2][2];
}

void CMatrix::Matrix3dMultByVec3d(matrix3d_t A, vec3d_t B, vec3d_t C)
{
	C[0] = B[0] * A[0][0] + B[1] * A[1][0] +
				B[2] * A[2][0];
	C[1] = B[0] * A[0][1] + B[1] * A[1][1] +
				B[2] * A[2][1];
	C[2] = B[0] * A[0][2] + B[1] * A[1][2] +
				B[2] * A[2][2];
}

void CMatrix::Matrix3FromAngles(vec3a_t angles, matrix3_t rotationMatrix)
{
	matrix3_t yawMatrix;
	matrix3_t pitchMatrix;
	matrix3_t rollMatrix;
	matrix3_t pitchRollMatrix;

#if 1
	memset(rollMatrix, 0, sizeof(rollMatrix));

	// Rotation about the x axis
	rollMatrix[0][0] = 1;
	rollMatrix[1][1] = cos(angles[PITCH]);
	rollMatrix[2][2] = rollMatrix[1][1];
	rollMatrix[1][2] = sin(angles[PITCH]);
	rollMatrix[2][1] = -rollMatrix[1][2];

	// Rotation about the y axis
	memset(pitchMatrix, 0, sizeof(pitchMatrix));

	pitchMatrix[0][0] = cos(angles[YAW]);
	pitchMatrix[2][2] = pitchMatrix[0][0];
	pitchMatrix[1][1] = 1;
	pitchMatrix[2][0] = sin(angles[YAW]);
	pitchMatrix[0][2] = -pitchMatrix[2][0];

	// Rotation about the z axis
	memset(yawMatrix, 0, sizeof(yawMatrix));

	yawMatrix[0][0] = cos(angles[ROLL]);
	yawMatrix[1][1] = yawMatrix[0][0];
	yawMatrix[0][1] = sin(angles[ROLL]);
	yawMatrix[1][0] = -yawMatrix[0][1];
	yawMatrix[2][2] = 1;

	Matrix3MultByMartrix3(pitchMatrix, rollMatrix, pitchRollMatrix);

	Matrix3MultByMartrix3(yawMatrix, pitchRollMatrix, rotationMatrix);
#else
	// Rotation about the y axis
	memset(pitchMatrix, 0, sizeof(pitchMatrix));

	pitchMatrix[0][0] = cos(angles[YAW]);
	pitchMatrix[2][2] = pitchMatrix[0][0];
	pitchMatrix[1][1] = 1;
	pitchMatrix[2][0] = sin(angles[YAW]);
	pitchMatrix[0][2] = -pitchMatrix[2][0];

	memset(rollMatrix, 0, sizeof(rollMatrix));

	// Rotation about the x axis
	rollMatrix[0][0] = 1;
	rollMatrix[1][1] = cos(angles[PITCH]);
	rollMatrix[2][2] = rollMatrix[1][1];
	rollMatrix[1][2] = sin(angles[PITCH]);
	rollMatrix[2][1] = -rollMatrix[1][2];

	// Rotation about the z axis
	memset(yawMatrix, 0, sizeof(yawMatrix));

	yawMatrix[0][0] = cos(angles[ROLL]);
	yawMatrix[1][1] = yawMatrix[0][0];
	yawMatrix[0][1] = sin(angles[ROLL]);
	yawMatrix[1][0] = -yawMatrix[0][1];
	yawMatrix[2][2] = 1;

	Matrix3MultByMartrix3(rollMatrix, pitchMatrix, pitchRollMatrix);

	Matrix3MultByMartrix3(yawMatrix, pitchRollMatrix, rotationMatrix);
#endif
}

void CMatrix::Matrix3dFromAngles(vec3a_t angles, matrix3d_t rotationMatrix)
{
	matrix3d_t yawMatrix;
	matrix3d_t pitchMatrix;
	matrix3d_t rollMatrix;
	matrix3d_t pitchRollMatrix;

	memset(rollMatrix, 0, sizeof(rollMatrix));

	// Rotation about the x axis
	rollMatrix[0][0] = 1;
	rollMatrix[1][1] = cos(angles[ROLL]);
	rollMatrix[2][2] = rollMatrix[1][1];
	rollMatrix[1][2] = sin(angles[ROLL]);
	rollMatrix[2][1] = -rollMatrix[1][2];

	// Rotation about the y axis
	memset(pitchMatrix, 0, sizeof(pitchMatrix));

	pitchMatrix[0][0] = cos(-angles[PITCH]);
	pitchMatrix[2][2] = pitchMatrix[0][0];
	pitchMatrix[1][1] = 1;
	pitchMatrix[2][0] = sin(-angles[PITCH]);
	pitchMatrix[0][2] = -pitchMatrix[2][0];

	// Rotation about the z axis
	memset(yawMatrix, 0, sizeof(yawMatrix));

	yawMatrix[0][0] = cos(angles[YAW]);
	yawMatrix[1][1] = yawMatrix[0][0];
	yawMatrix[0][1] = sin(angles[YAW]);
	yawMatrix[1][0] = -yawMatrix[0][1];
	yawMatrix[2][2] = 1;

	Matrix3MultByMartrix3(pitchMatrix, rollMatrix, pitchRollMatrix);

	Matrix3MultByMartrix3(yawMatrix, pitchRollMatrix, rotationMatrix);
}

void CMatrix::Matrixs3FromDirAndUp(vec3a_t direction, vec3a_t up, matrix3_t toLocal, matrix3_t fromLocal)
{
	matrix3d_t pitchYawMatrix;
	matrix3d_t rollMatrix;
	double r, s, roll;
	vec3a_t rotatedUp;
	float x, y, z;

	x = 1.0f;
	y = 1.0f;
	z = 1.0f;

	// need to check for direction == {x, 0, 0}
	// ot maybe not.  Looks like the math will work out to be the I or -I,
	// which seems right;  haven't actually seen any problems with it yet. . .

	r = 1-direction[0];
	s = sin(-acos(direction[0]));

#if 1
	// matrix from direction vector
	pitchYawMatrix[0][0] = direction[0]*x;
	pitchYawMatrix[1][0] = -direction[1]*s;
	pitchYawMatrix[2][0] = -direction[2]*s;
	pitchYawMatrix[0][1] = -pitchYawMatrix[1][0];
	pitchYawMatrix[1][1] = direction[2]*direction[2]*r+direction[0]*y;
	pitchYawMatrix[2][1] = -direction[1]*direction[2]*r;
	pitchYawMatrix[0][2] = direction[2]*s;
	pitchYawMatrix[1][2] = pitchYawMatrix[2][1];
	pitchYawMatrix[2][2] = direction[1]*direction[1]*r+direction[0]*z;

/*	pitchYawMatrix[0][0] = direction[0];
	pitchYawMatrix[1][0] = -direction[1]*s;
	pitchYawMatrix[2][0] = -direction[2]*s;
	pitchYawMatrix[0][1] = -pitchYawMatrix[1][0];
	pitchYawMatrix[1][1] = direction[2]*direction[2]*r+direction[0];
	pitchYawMatrix[2][1] = -direction[1]*direction[2]*r;
	pitchYawMatrix[0][2] = direction[2]*s;
	pitchYawMatrix[1][2] = pitchYawMatrix[2][1];
	pitchYawMatrix[2][2] = direction[1]*direction[1]*r+direction[0];*/
#else
	pitchYawMatrix[0][0] = 1;
	pitchYawMatrix[1][0] = 0;
	pitchYawMatrix[2][0] = 0;
	pitchYawMatrix[0][1] = 0;
	pitchYawMatrix[1][1] = 1;
	pitchYawMatrix[2][1] = 0;
	pitchYawMatrix[0][2] = 0;
	pitchYawMatrix[1][2] = 0;
	pitchYawMatrix[2][2] = 1;
#endif

	Matrix3dMultByVec3(pitchYawMatrix, up, &rotatedUp);

#ifndef NDEBUG
	{
		vec3a_t temp = { 1, 0, 0 };
		float dot;
		dot = Vec3DotProduct(rotatedUp, temp);
	}
#endif

	rotatedUp[0] = 0;

	Vec3Normalize(rotatedUp);

	roll = -(atan2(rotatedUp[2], rotatedUp[1])-ANGLE_90);

	memset(rollMatrix, 0, sizeof(rollMatrix));

#if 1
	// Rotation about the local x axis
	rollMatrix[0][0] = 1;
	rollMatrix[1][1] = cos(roll);
	rollMatrix[2][2] = rollMatrix[1][1];
	rollMatrix[1][2] = sin(roll);
	rollMatrix[2][1] = -rollMatrix[1][2];

#else
		// Rotation about the local x axis
		rollMatrix[0][0] = 1;
		rollMatrix[1][1] = 1;
		rollMatrix[2][2] = 1;
#endif

	Matrix3dMultByMartrix3d(rollMatrix, pitchYawMatrix, fromLocal);

	if(toLocal)
	{
		roll *= -1;

		memset(rollMatrix, 0, sizeof(rollMatrix));

#if 1
		// Rotation about the x axis
		rollMatrix[0][0] = 1;
		rollMatrix[1][1] = cos(roll);
		rollMatrix[2][2] = rollMatrix[1][1];
		rollMatrix[1][2] = sin(roll);
		rollMatrix[2][1] = -rollMatrix[1][2];
#else
		// Rotation about the local x axis
		rollMatrix[0][0] = 1;
		rollMatrix[1][1] = 1;
		rollMatrix[2][2] = 1;
#endif

//		direction[1] *= -1;
//		direction[2] *= -1;

		r = 1-direction[0];
		s = sin(acos(direction[0]));

#if 1
		// matrix from direction vector
		pitchYawMatrix[0][0] = direction[0]*x;
		pitchYawMatrix[1][0] = -direction[1]*s;
		pitchYawMatrix[2][0] = -direction[2]*s;
		pitchYawMatrix[0][1] = -pitchYawMatrix[1][0];
		pitchYawMatrix[1][1] = direction[2]*direction[2]*r+direction[0]*y;
		pitchYawMatrix[2][1] = -direction[1]*direction[2]*r;
		pitchYawMatrix[0][2] = direction[2]*s;
		pitchYawMatrix[1][2] = pitchYawMatrix[2][1];
		pitchYawMatrix[2][2] = direction[1]*direction[1]*r+direction[0]*z;
#else
		pitchYawMatrix[0][0] = 1;
		pitchYawMatrix[1][0] = 0;
		pitchYawMatrix[2][0] = 0;
		pitchYawMatrix[0][1] = 0;
		pitchYawMatrix[1][1] = 1;
		pitchYawMatrix[2][1] = 0;
		pitchYawMatrix[0][2] = 0;
		pitchYawMatrix[1][2] = 0;
		pitchYawMatrix[2][2] = 1;
#endif

		Matrix3dMultByVec3(pitchYawMatrix, up, &rotatedUp);

		Matrix3dMultByMartrix3d(pitchYawMatrix, rollMatrix, toLocal);

//		direction[1] *= -1;
//		direction[2] *= -1;
	}
}

#pragma optimize("p", on)

double CMatrix::Matricies3dFromDirAndUp(vec3a_t direction, vec3a_t up, matrix3d_t fromLocal, matrix3d_t toLocal)
{
	matrix3d_t pitchYawMatrix;
	matrix3d_t rollMatrix;
	double roll;
	vec3d_t rotatedUp, up_d;
	vec3d_t dir_d;

#ifndef NDEBUG
	double dot1, dot2;

#endif

	dir_d[0] = direction[0];
	dir_d[1] = direction[1];
	dir_d[2] = direction[2];

	up_d[0] = up[0];
	up_d[1] = up[1];
	up_d[2] = up[2];

#ifndef NDEBUG
	dot1 = Vec3dDotProduct(up_d, dir_d);
#endif

#if 0  // not sure what's wrong with this

	// need to check for direction == {x, 0, 0}
	// ot maybe not.  Looks like the math will work out to be the I or -I,
	// which seems right;  haven't actually seen any problems with it yet. . .

	r = 1-dir_d[0];
	s = sin(-acos(dir_d[0]));

	// matrix from dir_d vector
	pitchYawMatrix[0][0] = dir_d[0];
	pitchYawMatrix[1][0] = -dir_d[1]*s;
	pitchYawMatrix[2][0] = -dir_d[2]*s;
	pitchYawMatrix[0][1] = -pitchYawMatrix[1][0];
	pitchYawMatrix[1][1] = dir_d[2]*dir_d[2]*r+dir_d[0];
	pitchYawMatrix[2][1] = -dir_d[1]*dir_d[2]*r;
	pitchYawMatrix[0][2] = dir_d[2]*s;
	pitchYawMatrix[1][2] = pitchYawMatrix[2][1];
	pitchYawMatrix[2][2] = dir_d[1]*dir_d[1]*r+dir_d[0];
#else
	{
		double yaw, pitch;
		matrix3d_t pitchMatrix, yawMatrix;

		yaw = atan2(dir_d[1], dir_d[0]);
		pitch = asin(dir_d[2]);

		// Rotation about the y axis
		memset(pitchMatrix, 0, sizeof(pitchMatrix));

		pitchMatrix[0][0] = cos(pitch);
		pitchMatrix[2][2] = pitchMatrix[0][0];
		pitchMatrix[1][1] = 1;
		pitchMatrix[2][0] = sin(pitch);
		pitchMatrix[0][2] = -pitchMatrix[2][0];

		// Rotation about the z axis
		memset(yawMatrix, 0, sizeof(yawMatrix));

		yawMatrix[0][0] = cos(yaw);
		yawMatrix[1][1] = yawMatrix[0][0];
		yawMatrix[0][1] = sin(yaw);
		yawMatrix[1][0] = -yawMatrix[0][1];
		yawMatrix[2][2] = 1;

		Matrix3dMultByMartrix3d(pitchMatrix, yawMatrix, pitchYawMatrix);

	}
#endif

	Matrix3dMultByVec3d(pitchYawMatrix, up_d, rotatedUp);

#ifndef NDEBUG
	{
		vec3d_t xaxis = { 1, 0, 0 };
		double temp;

#if 0
		vec3d_t rotatedDir;
		double dot3, dot4;

		Matrix3dMultByVec3d(pitchYawMatrix, dir_d, rotatedDir);

		dot3 = Vec3dDotProduct(rotatedDir, xaxis);

		dot3 = acos(dot3);
		dot3 *= RAD_TO_ANGLE;

		dot4 = Vec3dDotProduct(dir_d, xaxis);

		dot4 = acos(dot4);
		dot4 *= RAD_TO_ANGLE;
#endif

		dot2 = Vec3dDotProduct(rotatedUp, xaxis);

		dot1 = acos(dot1);
		dot2 = acos(dot2);
		temp = (dot2-dot1)*RAD_TO_ANGLE;

#if 0
		assert(fabs(temp)/90.0 <= 0.1);	// greater than 10% error created by rotation of
										// up vector into world coordiantes
#else
		temp = fabs(temp);	// 0.1 for pelf, 0.024 for beetle
#endif
		dot1 *= RAD_TO_ANGLE;
		dot2 *= RAD_TO_ANGLE;
	}
#endif

	rotatedUp[0] = 0;

	Vec3dNormalize(rotatedUp);

	roll = -(atan2(rotatedUp[2], rotatedUp[1])-ANGLE_90);

	memset(rollMatrix, 0, sizeof(rollMatrix));

	// Rotation about the local x axis
	rollMatrix[0][0] = 1;
	rollMatrix[1][1] = cos(roll);
	rollMatrix[2][2] = rollMatrix[1][1];
	rollMatrix[1][2] = sin(roll);
	rollMatrix[2][1] = -rollMatrix[1][2];

	Matrix3dMultByMartrix3d(rollMatrix, pitchYawMatrix, fromLocal);

	// FIXME!!!!!!!
	// this is wrong, roll needs to rotate about the new local axis
	// I think the pitch yaw matix is still correct though
	if(toLocal)
	{
		roll *= -1;

#if 0

		memset(rollMatrix, 0, sizeof(rollMatrix));

#if 1
		// Rotation about the x axis
		rollMatrix[0][0] = 1;
		rollMatrix[1][1] = cos(roll);
		rollMatrix[2][2] = rollMatrix[1][1];
		rollMatrix[1][2] = sin(roll);
		rollMatrix[2][1] = -rollMatrix[1][2];
#else
		// Rotation about the local x axis
		rollMatrix[0][0] = 1;
		rollMatrix[1][1] = 1;
		rollMatrix[2][2] = 1;
#endif

#endif

#if 0	// not sure what's wrong with this!
		r = 1-direction[0];
		s = sin(acos(direction[0]));

		// matrix from direction vector
		pitchYawMatrix[0][0] = direction[0];
		pitchYawMatrix[1][0] = -direction[1]*s;
		pitchYawMatrix[2][0] = -direction[2]*s;
		pitchYawMatrix[0][1] = -pitchYawMatrix[1][0];
		pitchYawMatrix[1][1] = direction[2]*direction[2]*r+direction[0];
		pitchYawMatrix[2][1] = -direction[1]*direction[2]*r;
		pitchYawMatrix[0][2] = direction[2]*s;
		pitchYawMatrix[1][2] = pitchYawMatrix[2][1];
		pitchYawMatrix[2][2] = direction[1]*direction[1]*r+direction[0];
#else
	{
		double yaw, pitch;
		matrix3d_t pitchMatrix, yawMatrix;

		yaw = -atan2(dir_d[1], dir_d[0]);
		pitch = -asin(dir_d[2]);

		// Rotation about the y axis
		memset(pitchMatrix, 0, sizeof(pitchMatrix));

		pitchMatrix[0][0] = cos(pitch);
		pitchMatrix[2][2] = pitchMatrix[0][0];
		pitchMatrix[1][1] = 1;
		pitchMatrix[2][0] = sin(pitch);
		pitchMatrix[0][2] = -pitchMatrix[2][0];

		// Rotation about the z axis
		memset(yawMatrix, 0, sizeof(yawMatrix));

		yawMatrix[0][0] = cos(yaw);
		yawMatrix[1][1] = yawMatrix[0][0];
		yawMatrix[0][1] = sin(yaw);
		yawMatrix[1][0] = -yawMatrix[0][1];
		yawMatrix[2][2] = 1;

//		Matrix3dMultByMartrix3d(yawMatrix, pitchMatrix, pitchYawMatrix);
		Matrix3dMultByMartrix3d(yawMatrix, pitchMatrix, toLocal);
	}
#endif

//		Matrix3dMultByMartrix3d(pitchYawMatrix, rollMatrix, toLocal);
	}

	return roll;
}

float CMatrix::Matricies3FromDirAndUp(vec3a_t direction, vec3a_t up, matrix3_t fromLocal, matrix3_t toLocal)
{
	matrix3_t pitchYawMatrix;
	matrix3_t rollMatrix;
	float roll;
	vec3a_t rotatedUp, up_d;
	vec3a_t dir_d;

#ifndef NDEBUG
	float dot1, dot2;

#endif

	dir_d[0] = direction[0];
	dir_d[1] = direction[1];
	dir_d[2] = direction[2];

	up_d[0] = up[0];
	up_d[1] = up[1];
	up_d[2] = up[2];

#ifndef NDEBUG
	dot1 = Vec3DotProduct(up_d, dir_d);
#endif

#if 0  // not sure what's wrong with this

	// need to check for direction == {x, 0, 0}
	// ot maybe not.  Looks like the math will work out to be the I or -I,
	// which seems right;  haven't actually seen any problems with it yet. . .

	r = 1-dir_d[0];
	s = sin(-acos(dir_d[0]));

	// matrix from dir_d vector
	pitchYawMatrix[0][0] = dir_d[0];
	pitchYawMatrix[1][0] = -dir_d[1]*s;
	pitchYawMatrix[2][0] = -dir_d[2]*s;
	pitchYawMatrix[0][1] = -pitchYawMatrix[1][0];
	pitchYawMatrix[1][1] = dir_d[2]*dir_d[2]*r+dir_d[0];
	pitchYawMatrix[2][1] = -dir_d[1]*dir_d[2]*r;
	pitchYawMatrix[0][2] = dir_d[2]*s;
	pitchYawMatrix[1][2] = pitchYawMatrix[2][1];
	pitchYawMatrix[2][2] = dir_d[1]*dir_d[1]*r+dir_d[0];
#else
	{
		float yaw, pitch;
		matrix3_t pitchMatrix, yawMatrix;

		yaw = atan2(dir_d[1], dir_d[0]);
		pitch = asin(dir_d[2]);

		// Rotation about the y axis
		memset(pitchMatrix, 0, sizeof(pitchMatrix));

		pitchMatrix[0][0] = cos(pitch);
		pitchMatrix[2][2] = pitchMatrix[0][0];
		pitchMatrix[1][1] = 1;
		pitchMatrix[2][0] = sin(pitch);
		pitchMatrix[0][2] = -pitchMatrix[2][0];

		// Rotation about the z axis
		memset(yawMatrix, 0, sizeof(yawMatrix));

		yawMatrix[0][0] = cos(yaw);
		yawMatrix[1][1] = yawMatrix[0][0];
		yawMatrix[0][1] = sin(yaw);
		yawMatrix[1][0] = -yawMatrix[0][1];
		yawMatrix[2][2] = 1;

		Matrix3MultByMartrix3(pitchMatrix, yawMatrix, pitchYawMatrix);

	}
#endif

	Matrix3MultByVec3(pitchYawMatrix, up_d, rotatedUp);

#ifndef NDEBUG
	{
		vec3a_t xaxis = { 1, 0, 0 };
		float temp;

#if 0
		vec3a_t rotatedDir;
		float dot3, dot4;

		Matrix3MultByVec3(pitchYawMatrix, dir_d, rotatedDir);

		dot3 = Vec3DotProduct(rotatedDir, xaxis);

		dot3 = acos(dot3);
		dot3 *= RAD_TO_ANGLE;

		dot4 = Vec3DotProduct(dir_d, xaxis);

		dot4 = acos(dot4);
		dot4 *= RAD_TO_ANGLE;
#endif

		dot2 = Vec3DotProduct(rotatedUp, xaxis);

		dot1 = acos(dot1);
		dot2 = acos(dot2);
		temp = (dot2-dot1)*RAD_TO_ANGLE;

#if 0
		assert(fabs(temp)/90.0 <= 0.1);	// greater than 10% error created by rotation of
										// up vector into world coordiantes
#else
		temp = fabs(temp);	// 0.1 for pelf, 0.024 for beetle
#endif
		dot1 *= RAD_TO_ANGLE;
		dot2 *= RAD_TO_ANGLE;
	}
#endif

	rotatedUp[0] = 0;

	Vec3Normalize(rotatedUp);

	roll = -(atan2(rotatedUp[2], rotatedUp[1])-ANGLE_90);

	memset(rollMatrix, 0, sizeof(rollMatrix));

	// Rotation about the local x axis
	rollMatrix[0][0] = 1;
	rollMatrix[1][1] = cos(roll);
	rollMatrix[2][2] = rollMatrix[1][1];
	rollMatrix[1][2] = sin(roll);
	rollMatrix[2][1] = -rollMatrix[1][2];

	Matrix3MultByMartrix3(rollMatrix, pitchYawMatrix, fromLocal);

	// FIXME!!!!!!!
	// this is wrong, roll needs to rotate about the new local axis
	// I think the pitch yaw matix is still correct though
	if(toLocal)
	{
		roll *= -1;

#if 0

		memset(rollMatrix, 0, sizeof(rollMatrix));

#if 1
		// Rotation about the x axis
		rollMatrix[0][0] = 1;
		rollMatrix[1][1] = cos(roll);
		rollMatrix[2][2] = rollMatrix[1][1];
		rollMatrix[1][2] = sin(roll);
		rollMatrix[2][1] = -rollMatrix[1][2];
#else
		// Rotation about the local x axis
		rollMatrix[0][0] = 1;
		rollMatrix[1][1] = 1;
		rollMatrix[2][2] = 1;
#endif

#endif

#if 0	// not sure what's wrong with this!
		r = 1-direction[0];
		s = sin(acos(direction[0]));

		// matrix from direction vector
		pitchYawMatrix[0][0] = direction[0];
		pitchYawMatrix[1][0] = -direction[1]*s;
		pitchYawMatrix[2][0] = -direction[2]*s;
		pitchYawMatrix[0][1] = -pitchYawMatrix[1][0];
		pitchYawMatrix[1][1] = direction[2]*direction[2]*r+direction[0];
		pitchYawMatrix[2][1] = -direction[1]*direction[2]*r;
		pitchYawMatrix[0][2] = direction[2]*s;
		pitchYawMatrix[1][2] = pitchYawMatrix[2][1];
		pitchYawMatrix[2][2] = direction[1]*direction[1]*r+direction[0];
#else
	{
		float yaw, pitch;
		matrix3_t pitchMatrix, yawMatrix;

		yaw = -atan2(dir_d[1], dir_d[0]);
		pitch = -asin(dir_d[2]);

		// Rotation about the y axis
		memset(pitchMatrix, 0, sizeof(pitchMatrix));

		pitchMatrix[0][0] = cos(pitch);
		pitchMatrix[2][2] = pitchMatrix[0][0];
		pitchMatrix[1][1] = 1;
		pitchMatrix[2][0] = sin(pitch);
		pitchMatrix[0][2] = -pitchMatrix[2][0];

		// Rotation about the z axis
		memset(yawMatrix, 0, sizeof(yawMatrix));

		yawMatrix[0][0] = cos(yaw);
		yawMatrix[1][1] = yawMatrix[0][0];
		yawMatrix[0][1] = sin(yaw);
		yawMatrix[1][0] = -yawMatrix[0][1];
		yawMatrix[2][2] = 1;

		//Matrix3dMultByMartrix3d(yawMatrix, pitchMatrix, pitchYawMatrix);
		Matrix3MultByMartrix3(yawMatrix, pitchMatrix, toLocal);
	}
#endif

		//Matrix3dMultByMartrix3d(pitchYawMatrix, rollMatrix, toLocal);
	}

	return roll;
}

#pragma optimize("p", off)

void CMatrix::RotatePointAboutLocalOrigin(matrix3_t rotation, vec3a_t origin, vec3a_t point)
{
	vec3a_t temp;

	point[0] -= origin[0];
	point[1] -= origin[1];
	point[2] -= origin[2];

	Matrix3MultByVec3(rotation, point, temp);

	point[0] = temp[0] + origin[0];
	point[1] = temp[1] + origin[1];
	point[2] = temp[2] + origin[2];
}

void CMatrix::RotatePointAboutLocalOrigin_d(matrix3d_t rotation, vec3a_t origin, vec3a_t point)
{
	vec3a_t temp;

	point[0] -= origin[0];
	point[1] -= origin[1];
	point[2] -= origin[2];

	Matrix3dMultByVec3(rotation, point, &temp);

	point[0] = temp[0] + origin[0];
	point[1] = temp[1] + origin[1];
	point[2] = temp[2] + origin[2];
}
