typedef float vec3a_t[3];
typedef double vec3d_t[3];
typedef float matrix3_t[3][3];
typedef float matrix3d_t[3][3];

class CMatrix
{
public:

	static void Matrix3dMultByVec3(matrix3d_t A, vec3a_t B, vec3a_t *C);
	static void Matrix3MultByMartrix3(matrix3_t A, matrix3_t B, matrix3_t C);
	static void Matrix3MultByVec3(matrix3_t A, vec3a_t B, vec3a_t C);
	static void Matrix3FromAngles(vec3a_t angles, matrix3_t rotationMatrix);
	static void Matrixs3FromDirAndUp(vec3a_t direction, vec3a_t up, matrix3_t toLocal, matrix3_t fromLocal);
	static void RotatePointAboutLocalOrigin(matrix3_t rotation, vec3a_t origin, vec3a_t point);

	static void Matrix3dMultByMartrix3d(matrix3d_t A, matrix3d_t B, matrix3d_t C);
	static void Matrix3dMultByVec3d(matrix3d_t A, vec3d_t B, vec3d_t C);
	static void Matrix3dFromAngles(vec3a_t angles, matrix3d_t rotationMatrix);
	static double Matricies3dFromDirAndUp(vec3a_t direction, vec3a_t up, matrix3_t toWorld, matrix3_t partialToLocal);
	static void RotatePointAboutLocalOrigin_d(matrix3d_t rotation, vec3a_t origin, vec3a_t point);

	static float Matricies3FromDirAndUp(vec3a_t direction, vec3a_t up, matrix3_t fromLocal, matrix3_t toLocal);
};
