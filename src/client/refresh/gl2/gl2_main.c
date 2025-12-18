/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_main.c
#include "r_local.h"

void R_Clear (void);

model_t		*r_worldmodel = NULL;
model_t		*r_defaultmodel = NULL; //for missing models

float		gldepthmin, gldepthmax;


glconfig_t gl_config;
glstate_t  gl_state;

image_t		*r_texture_white;
image_t		*r_texture_missing;		// use for bad textures
image_t		*r_texture_particle;	// little dot for particles
image_t		*r_texture_view;

rentity_t	*pCurrentRefEnt;
model_t		*pCurrentModel;

cplane_t	frustum[4];

int			r_visframecount;	// bumped when going to a new PVS
int			r_framecount;		// used for dlight push checking

rperfcounters_t rperf;

static rgba_t	v_blend;			// final blending color

extern vec3_t	model_shadevector;
extern vec3_t	model_shadelight;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;

mat4_t	r_projection_matrix;
mat4_t	r_ortho_matrix;
mat4_t	r_world_matrix;
mat4_t	r_local_matrix;

vertexbuffer_t* vb_particles;
//
// screen size info
//
refdef_t	r_newrefdef;
viddef_t	vid; // TODO get rid of this

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

extern void R_DrawStringOld(char* string, float x, float y, float fontSize, int alignx, rgba_t color);
extern void R_ClearFBO();
extern void R_RenderToFBO(qboolean enable);
extern void R_DrawDebugLines(void);
extern void R_DrawEntityModel(rentity_t* ent);

extern void R_DrawText(int x, int y, int alignX, int fontId, float scale, vec4_t color, char* text);
extern int R_GetFontHeight(int fontId);

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;

	if (r_nocull->value)
		return false;

	for (i = 0; i < 4; i++)
		if ( BOX_ON_PLANE_SIDE(mins, maxs, &frustum[i]) == 2)
			return true;

	return false;
}

/*
=================
R_RotateForEntity
=================
*/
void R_RotateForEntity (rentity_t *e)
{
	Mat4MakeIdentity(r_local_matrix);
	Mat4Translate(r_local_matrix, e->origin[0], e->origin[1], e->origin[2]);
	Mat4RotateAroundZ(r_local_matrix, e->angles[1]);

#ifdef FIX_SQB
	Mat4RotateAroundY(r_local_matrix, e->angles[0]);
	Mat4RotateAroundX(r_local_matrix, e->angles[2]);
#else
	Mat4RotateAroundY(r_local_matrix, -e->angles[0]); // stupid quake bug
	Mat4RotateAroundX(r_local_matrix, -e->angles[2]); // stupid quake bug
#endif
}


/*
=============
R_DrawNullModel

Draw the default model
=============
*/
static void R_DrawNullModel (void)
{
#if 0 //TODO need to do something here
	vec3_t	shadelight;
	int		i;

	if (pCurrentRefEnt->renderfx & RF_FULLBRIGHT)
		VectorSet(model_shadelight, 1.0f, 1.0f, 1.0f);
	else
		R_LightPoint (pCurrentRefEnt->origin, shadelight);

	R_RotateForEntity (pCurrentRefEnt);

	glDisable (GL_TEXTURE_2D);
	glColor3fv (shadelight);

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (0, 0, -16);
	for (i=0 ; i<=4 ; i++)
		glVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	glEnd ();

	glBegin (GL_TRIANGLE_FAN);
	glVertex3f (0, 0, 16);
	for (i=4 ; i>=0 ; i--)
		glVertex3f (16*cos(i*M_PI/2), 16*sin(i*M_PI/2), 0);
	glEnd ();

	glColor3f (1,1,1);
	glEnable (GL_TEXTURE_2D);
#endif
}


/*
=================
R_DrawCurrentEntity
=================
*/
static inline void R_DrawCurrentEntity()
{
	if (pCurrentRefEnt->renderfx & RF_BEAM)
	{
		R_DrawBeam(pCurrentRefEnt);
	}
	else
	{
		if (!pCurrentModel)
		{
			R_DrawNullModel();
		}
		else
		{
			if (pCurrentModel->type == MOD_BRUSH)
			{
				R_DrawBrushModel(pCurrentRefEnt);
			}
			else
			{
				R_DrawEntityModel(pCurrentRefEnt);
			}
		}
	}
}

/*
=============
R_DrawEntitiesOnList
=============
*/
void R_DrawEntitiesOnList (void)
{
	int		i;

	if (!r_drawentities->value)
		return;

	// draw non-transparent first
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		pCurrentRefEnt = &r_newrefdef.entities[i];
		pCurrentModel = pCurrentRefEnt->model;
		if ((pCurrentRefEnt->renderfx & RF_TRANSLUCENT))
			continue;	// reject transparent
		R_DrawCurrentEntity();
	}


	// draw transparent entities
	R_WriteToDepthBuffer(GL_FALSE);	// no z writes
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		pCurrentRefEnt = &r_newrefdef.entities[i];
		pCurrentModel = pCurrentRefEnt->model;
		if (!(pCurrentRefEnt->renderfx & RF_TRANSLUCENT))
			continue;	// reject solid
		R_DrawCurrentEntity();
	}
	R_WriteToDepthBuffer(GL_TRUE);	// reenable z writing

	R_UnbindProgram();
}

/*
===============
R_DrawParticles
===============
*/
void R_DrawParticles( int num_particles, const particle_t particles[] )
{
	const particle_t *p;
	int				i;
	vec3_t			up, right;
	int vertcnt = 0;

	for (p = particles, i = 0; i < num_particles ; i++, p++)
	{
		if (p->size[0] > 0.0f && p->size[1] > 0.0f)
		{
			VectorScale(vup, p->size[0], up);
			VectorScale(vright, p->size[1], right);
		}
		else
		{
			VectorScale(vup, 3.0f, up);
			VectorScale(vright, 3.0f, right);
		}

		Vector4Set(vb_particles->verts[vertcnt].rgba, p->color[0], p->color[1], p->color[2], p->alpha);
		Vector2Set(vb_particles->verts[vertcnt].st, 0.0625, 0.0625 );
		VectorCopy(p->origin, vb_particles->verts[vertcnt].xyz);
		vertcnt++;

		Vector4Set(vb_particles->verts[vertcnt].rgba, p->color[0], p->color[1], p->color[2], p->alpha);
		Vector2Set(vb_particles->verts[vertcnt].st, 1.0625, 0.0625 );
		VectorSet(vb_particles->verts[vertcnt].xyz, p->origin[0] + up[0], p->origin[1] + up[1], p->origin[2] + up[2]);
		vertcnt++;

		Vector4Set(vb_particles->verts[vertcnt].rgba, p->color[0], p->color[1], p->color[2], p->alpha);
		Vector2Set(vb_particles->verts[vertcnt].st, 0.0625, 1.0625 );
		VectorSet(vb_particles->verts[vertcnt].xyz, p->origin[0] + right[0], p->origin[1] + right[1], p->origin[2] + right[2]);
		vertcnt++;
	}

	R_UpdateVertexBuffer(vb_particles, NULL, vertcnt, (V_UV|V_COLOR|V_NOFREE));

	R_BindProgram(GLPROG_PARTICLE);
	R_MultiTextureBind(TMU_DIFFUSE, r_texture_particle->texnum);
	//R_BindTexture(r_texture_white->texnum); // testing

	R_Blend(true);
	R_WriteToDepthBuffer(GL_FALSE);		// no z buffering

	R_DrawVertexBuffer(vb_particles, 0, 0);

	R_Blend(false);
	R_WriteToDepthBuffer(GL_TRUE);		// back to normal Z buffering
	R_UnbindProgram();
}


//=======================================================================

/*
============
SignbitsForPlane
============
*/
static int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}

/*
============
R_SetFrustum
============
*/
static void R_SetFrustum()
{
	int		i;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.view.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.view.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.view.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.view.fov_y / 2 ) );

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
static void R_SetupFrame()
{
	mleaf_t	*leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.view.origin, r_origin);

	AngleVectors (r_newrefdef.view.angles, vpn, vright, vup);

// current viewcluster
	if ( !( r_newrefdef.view.flags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		leaf = Mod_BSP_PointInLeaf (r_origin, r_worldmodel);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			leaf = Mod_BSP_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			leaf = Mod_BSP_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	Vector4Copy(r_newrefdef.view.fx.blend, v_blend);

	// clear out the portion of the screen that the NOWORLDMODEL defines
	if ( r_newrefdef.view.flags & RDF_NOWORLDMODEL )
	{
		glEnable( GL_SCISSOR_TEST );
		glClearColor( 0.3, 0.3, 0.3, 1 );
		glScissor( r_newrefdef.x, vid.height - r_newrefdef.height - r_newrefdef.y, r_newrefdef.width, r_newrefdef.height );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glClearColor( 1, 0, 0.5, 0.5 );
		glDisable( GL_SCISSOR_TEST );
	}
}

/*
===============
MYgluPerspective
===============
*/
void MYgluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	xmin += -( 2 * gl_state.camera_separation ) / zNear;
	xmax += -( 2 * gl_state.camera_separation ) / zNear;

	mat4_t mat;
	Mat4Perspective(mat, xmin, xmax, ymin, ymax, zNear, zFar);
	memcpy(r_projection_matrix, mat, sizeof(mat));
}

/*
=============
R_SetupGL
=============
*/
static void R_SetupGL()
{
	float	screenaspect;
//	float	yfov;
	int		x, x2, y2, y, w, h;

	//
	// set up viewport
	//
	x = floor(r_newrefdef.x * vid.width / vid.width);
	x2 = ceil((r_newrefdef.x + r_newrefdef.width) * vid.width / vid.width);
	y = floor(vid.height - r_newrefdef.y * vid.height / vid.height);
	y2 = ceil(vid.height - (r_newrefdef.y + r_newrefdef.height) * vid.height / vid.height);

	w = x2 - x;
	h = y - y2;

	glViewport (x, y2, w, h);

	//
	// set up projection matrix
	//
    screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
    MYgluPerspective (r_newrefdef.view.fov_y,  screenaspect,  4, 4096);

	R_SetCullFace(GL_FRONT);

	mat4_t mat;
	Mat4MakeIdentity(mat);

	Mat4RotateAroundX(mat, -90);	// put Z going up
	Mat4RotateAroundZ(mat, 90);	// put Z going up
	Mat4RotateAroundX(mat, -r_newrefdef.view.angles[2]);
	Mat4RotateAroundY(mat, -r_newrefdef.view.angles[0]);
	Mat4RotateAroundZ(mat, -r_newrefdef.view.angles[1]);
	Mat4Translate(mat, -r_newrefdef.view.origin[0], -r_newrefdef.view.origin[1], -r_newrefdef.view.origin[2]);

	memcpy(r_world_matrix, mat, sizeof(mat));

	//only send the matricies once since they'll never change during a frame.
	R_BindProgram(GLPROG_WORLD);

	//
	// set drawing parms
	//
	R_CullFace(r_cull->value);

	R_Blend(false);
	R_AlphaTest(false);
	R_DepthTest(true);
}

/*
=============
R_Clear
=============
*/
void R_Clear (void)
{
	if (r_clear->value)
		glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else
		glClear (GL_DEPTH_BUFFER_BIT);

	gldepthmin = 0;
	gldepthmax = 1;

	glDepthFunc (GL_LEQUAL);
	glDepthRange (gldepthmin, gldepthmax);
}


/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/
void R_RenderView (refdef_t *fd)
{
	if (r_norefresh->value)
		return;

	r_newrefdef = *fd;

	if (!r_worldmodel && !( r_newrefdef.view.flags & RDF_NOWORLDMODEL ) )
		ri.Error (ERR_DROP, "R_RenderView: NULL worldmodel");

	// clear performance counters
	rperf.brush_polys = 0;
	rperf.brush_tris = 0;
	rperf.brush_drawcalls = 0;
	rperf.alias_tris = 0;
	rperf.alias_drawcalls = 0;

	for(int i = 0; i < MIN_TEXTURE_MAPPING_UNITS; i++)
		rperf.texture_binds[i] = 0;

	R_StartProfiling();
	R_PushDlights ();

	if (r_finish->value)
		glFinish ();

	R_SetupFrame ();

	R_SetFrustum ();

	R_SetupGL ();

	R_World_MarkLeaves ();	// done here so we know if we're in water

	R_UpdateCommonProgUniforms(false);

	R_ClearFBO(); 
	R_RenderToFBO(true); // begin rendering to fbo
	R_ProfileAtStage(STAGE_SETUP);

	R_DrawWorld();
	R_ProfileAtStage(STAGE_DRAWWORLD);

	R_DrawEntitiesOnList();
	R_ProfileAtStage(STAGE_ENTITIES);

	R_DrawDebugLines();
	R_ProfileAtStage(STAGE_DEBUG);

//	R_RenderDlights ();
	R_World_DrawAlphaSurfaces();
	R_ProfileAtStage(STAGE_ALPHASURFS);

	R_DrawParticles(r_newrefdef.num_particles, r_newrefdef.particles);
	R_ProfileAtStage(STAGE_PARTICLES);

	R_RenderToFBO(false); // end rendering to fbo
	
	R_SelectTextureUnit(0);
	if (r_speeds->value == 1.0f)
	{
		ri.Printf (PRINT_ALL, "%4i bsppolys, %4i mdltris, %i vistex, %i texbinds, %i lmbinds,\n",
			rperf.brush_polys,
			rperf.alias_tris,
			rperf.brush_textures,
			rperf.texture_binds[TMU_DIFFUSE],
			rperf.texture_binds[TMU_LIGHTMAP]);
	}

	R_ProfileAtStage(STAGE_TOTAL);
	R_FinishProfiling();
}

/*
================
R_BeginOrthoProjection
================
*/
void R_BeginOrthoProjection()
{
	// set 2D virtual screen size
	glViewport (0,0, vid.width, vid.height);
	Mat4Ortho(r_ortho_matrix, 0, vid.width, vid.height, 0, -99999, 99999);
	R_UpdateCommonProgUniforms(true);
	R_DepthTest(false);
	R_CullFace(false);
	R_Blend(false);
	R_AlphaTest(true);
}

void R_NewDrawFill(rect_t pos, rgba_t color);
static void R_DrawPerfCounters()
{
	float x, y, h;
	vec4_t color;
	float fontscale;

	if (r_speeds->value <= 1.0f)
		return;

	Vector4Set(color, 0, 0, 0, 0.35f);
	
	R_ProgUniform4f(LOC_COLOR4, 0, 0, 0, 0.5);
	R_DrawFill(vid.width-175, 42, 175, 220);

	fontscale = 0.25;
	x = vid.width - 10;
	y = 46;
	h = R_GetFontHeight(0) * fontscale;

	Vector4Set(color, 1.0, 0.65, 0, 1.0);
	R_DrawText(x + 5, h + 4, 2, 0, fontscale, color, va("%s", gl_config.renderer_string));

	Vector4Set(color, 1, 1, 1, 1.0);
	R_DrawText(x, y, 2, 0, fontscale, color, va("%i brush polygons", rperf.brush_polys));

	if (rperf.brush_tris > 0)
	{
		R_DrawText(x, y += h, 2, 0, fontscale, color, va("%i brush triangles", rperf.brush_tris));
	}

	R_DrawText(x, y += h, 2, 0, fontscale, color, va("%i brush drawcalls", rperf.brush_drawcalls));

	R_DrawText(x, y += h, 2, 0, fontscale, color, va("%i textures in chain", rperf.brush_textures));
	R_DrawText(x, y += h, 2, 0, fontscale, color, va("%i lightmap binds", rperf.texture_binds[TMU_LIGHTMAP]));
	R_DrawText(x, y += h, 2, 0, fontscale, color, va("%i texture binds", rperf.texture_binds[TMU_DIFFUSE]));

	Vector4Set(color, 0.8, 0.8, 1, 1.0);
	R_DrawText(x, y += h*2, 2, 0, fontscale, color, va("%i dynamic lights", r_newrefdef.num_dlights));
	R_DrawText(x, y += h, 2, 0, fontscale, color, va("%i render entities", r_newrefdef.num_entities));
	R_DrawText(x, y += h, 2, 0, fontscale, color, va("%i particles count", r_newrefdef.num_particles));

	Vector4Set(color, 1, 1, 1, 1.0);
	R_DrawText(x, y += h * 2, 2, 0, fontscale, color, va("%i rendered models", rperf.alias_drawcalls));
	R_DrawText(x, y += h, 2, 0, fontscale, color, va("%i model tris total", rperf.alias_tris));

	R_DrawProfilingReport();
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
void R_RenderFrame (refdef_t *fd, qboolean onlyortho)
{

	//
	// render world into framebuffer
	//
	if (!onlyortho)
	{
		R_RenderView(fd);
	}

	//
	// enter 2d mode
	//
	R_BeginOrthoProjection (); //yes this uploads the uniforms twice but sometimes beginframe isn't called or something

	if (!onlyortho)
	{
		R_BindProgram(GLPROG_POSTFX);
		R_ProgUniform4f(LOC_COLOR4, v_blend[0], v_blend[1], v_blend[2], v_blend[3]);

		// draw the frame buffer
		R_DrawFBO(0, 0, r_newrefdef.width, r_newrefdef.height, true);
	}

	extern void R_DrawText(int x, int y, int alignX, int fontId, float scale, vec4_t color, char* text);

	// begin GUI rendering

	R_BindProgram(GLPROG_GUI);
	R_DrawPerfCounters();

	//vec4_t c = { 1,1,0,1 };
	//R_DrawText(vid.width / 2, 200 + 64 * 1, 1, 0, 1.0, c, "01234567890\n!@#$%^&\n*()_+[{]}\|-=;:',<.>?");

}

/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame( float camera_separation )
{

	gl_state.camera_separation = camera_separation;

	/*
	** change modes if necessary
	*/
	if ( r_mode->modified || r_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		cvar_t	*ref;

		ref = ri.Cvar_Get ("r_renderer", DEFAULT_RENDERER, 0);
		ref->modified = true;
	}

	if ( r_gamma->modified )
	{
		r_gamma->modified = false;

		if (r_gamma->value < 0.3)
			ri.Cvar_Set("r_gamma", "0.3");
		else if (r_gamma->value > 2)
			ri.Cvar_Set("r_gamma", "2");
	}

	if (r_intensity->modified)
	{
		r_intensity->modified = false;

		if (r_intensity->value < 1)
			ri.Cvar_Set("r_intensity", "1");
		else if (r_intensity->value > 6)
			ri.Cvar_Set("r_intensity", "6");
	}

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	R_BeginOrthoProjection();

	/*
	** draw buffer stuff
	*/
	if ( r_drawbuffer->modified )
	{
		r_drawbuffer->modified = false;

		if ( gl_state.camera_separation == 0 || !gl_state.stereo_enabled )
		{
			if ( Q_stricmp( r_drawbuffer->string, "GL_FRONT" ) == 0 )
				glDrawBuffer( GL_FRONT );
			else
				glDrawBuffer( GL_BACK );
		}
	}

	/*
	** texturemode stuff
	*/
	if ( r_texturemode->modified )
	{
		R_SetTextureMode( r_texturemode->string );
		r_texturemode->modified = false;
	}

	if ( r_texturealphamode->modified )
	{
		R_SetTextureAlphaMode( r_texturealphamode->string );
		r_texturealphamode->modified = false;
	}

	if ( r_texturesolidmode->modified )
	{
		R_TextureSolidMode( r_texturesolidmode->string );
		r_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
** R_DrawBeam
*/
void R_DrawBeam( rentity_t *e )
{
#define NUM_BEAM_SEGS 6

	int	i;
	float r, g, b;

	vec3_t perpvec;
	vec3_t direction, normalized_direction;
	vec3_t	start_points[NUM_BEAM_SEGS], end_points[NUM_BEAM_SEGS];
	vec3_t oldorigin, origin;

	oldorigin[0] = e->oldorigin[0];
	oldorigin[1] = e->oldorigin[1];
	oldorigin[2] = e->oldorigin[2];

	origin[0] = e->origin[0];
	origin[1] = e->origin[1];
	origin[2] = e->origin[2];

	normalized_direction[0] = direction[0] = oldorigin[0] - origin[0];
	normalized_direction[1] = direction[1] = oldorigin[1] - origin[1];
	normalized_direction[2] = direction[2] = oldorigin[2] - origin[2];

	if ( VectorNormalize( normalized_direction ) == 0 )
		return;

	PerpendicularVector( perpvec, normalized_direction );
	VectorScale( perpvec, e->frame / 2, perpvec );

	for ( i = 0; i < 6; i++ )
	{
		RotatePointAroundVector( start_points[i], normalized_direction, perpvec, (360.0/NUM_BEAM_SEGS)*i );
		VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[i], direction, end_points[i] );
	}

	glDisable( GL_TEXTURE_2D );
	R_Blend(true);
	R_WriteToDepthBuffer(GL_FALSE);

//	r = ( d_8to24table[e->skinnum & 0xFF] ) & 0xFF;
//	g = ( d_8to24table[e->skinnum & 0xFF] >> 8 ) & 0xFF;
//	b = ( d_8to24table[e->skinnum & 0xFF] >> 16 ) & 0xFF;

	r = g = b = 1.0f; // FIXME!
	r *= 1/255.0F;
	g *= 1/255.0F;
	b *= 1/255.0F;

	R_ProgUniform4f(LOC_COLOR4, r, g, b, e->alpha );

	glBegin( GL_TRIANGLE_STRIP );
	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		glVertex3fv( start_points[i] );
		glVertex3fv( end_points[i] );
		glVertex3fv( start_points[(i+1)%NUM_BEAM_SEGS] );
		glVertex3fv( end_points[(i+1)%NUM_BEAM_SEGS] );
	}
	glEnd();

	glEnable( GL_TEXTURE_2D );
	R_Blend(false);
	R_WriteToDepthBuffer(GL_TRUE);
}

