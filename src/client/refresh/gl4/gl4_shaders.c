/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016-2017 Daniel Gibson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * OpenGL4 refresher: Handling shaders
 *
 * =======================================================================
 */

#include "header/local.h"

// TODO: remove eprintf() usage
#define eprintf(...)  R_Printf(PRINT_ALL, __VA_ARGS__)


static GLuint
CompileShader(GLenum shaderType, const char* shaderSrc, const char* shaderSrc2)
{
	GLuint shader = glCreateShader(shaderType);
	const char* version = glshader_version(gl4config.major_version, gl4config.minor_version);
	const char* sources[3] = { version, shaderSrc, shaderSrc2 };
	int numSources = shaderSrc2 != NULL ? 3 : 2;

	glShaderSource(shader, numSources, sources, NULL);
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status != GL_TRUE)
	{
		char buf[2048];
		char* bufPtr = buf;
		int bufLen = sizeof(buf);
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength >= bufLen)
		{
			bufPtr = malloc(infoLogLength+1);
			bufLen = infoLogLength+1;
			if (bufPtr == NULL)
			{
				bufPtr = buf;
				bufLen = sizeof(buf);
				eprintf("WARN: In CompileShader(), malloc(%d) failed!\n", infoLogLength+1);
			}
		}

		glGetShaderInfoLog(shader, bufLen, NULL, bufPtr);

		const char* shaderTypeStr = "";
		switch(shaderType)
		{
			case GL_VERTEX_SHADER:   shaderTypeStr = "vertex"; break;
			case GL_FRAGMENT_SHADER: shaderTypeStr = "fragment"; break;
			case GL_COMPUTE_SHADER:  shaderTypeStr = "compute"; break;
		}
		eprintf("ERROR: Compiling %s Shader failed: %s\n", shaderTypeStr, bufPtr);
		glDeleteShader(shader);

		if (bufPtr != buf)  free(bufPtr);

		return 0;
	}

	return shader;
}

static GLuint
CreateShaderProgram(int numShaders, const GLuint* shaders)
{
	int i=0;
	GLuint shaderProgram = glCreateProgram();
	if (shaderProgram == 0)
	{
		eprintf("ERROR: Couldn't create a new Shader Program!\n");
		return 0;
	}

	for (i=0; i<numShaders; ++i)
	{
		glAttachShader(shaderProgram, shaders[i]);
	}

	// make sure all shaders use the same attribute locations for common attributes
	// (so the same VAO can easily be used with different shaders)
	glBindAttribLocation(shaderProgram, GL4_ATTRIB_POSITION, "position");
	glBindAttribLocation(shaderProgram, GL4_ATTRIB_TEXCOORD, "texCoord");
	glBindAttribLocation(shaderProgram, GL4_ATTRIB_LMTEXCOORD, "lmTexCoord");
	glBindAttribLocation(shaderProgram, GL4_ATTRIB_COLOR, "vertColor");
	glBindAttribLocation(shaderProgram, GL4_ATTRIB_NORMAL, "normal");
	glBindAttribLocation(shaderProgram, GL4_ATTRIB_LIGHTFLAGS, "lightFlags");

	// the following line is not necessary/implicit (as there's only one output)
	// glBindFragDataLocation(shaderProgram, 0, "outColor"); XXX would this even be here?

	glLinkProgram(shaderProgram);

	GLint status;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{
		char buf[2048];
		char* bufPtr = buf;
		int bufLen = sizeof(buf);
		GLint infoLogLength;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength >= bufLen)
		{
			bufPtr = malloc(infoLogLength+1);
			bufLen = infoLogLength+1;
			if (bufPtr == NULL)
			{
				bufPtr = buf;
				bufLen = sizeof(buf);
				eprintf("WARN: In CreateShaderProgram(), malloc(%d) failed!\n", infoLogLength+1);
			}
		}

		glGetProgramInfoLog(shaderProgram, bufLen, NULL, bufPtr);

		eprintf("ERROR: Linking shader program failed: %s\n", bufPtr);

		glDeleteProgram(shaderProgram);

		if (bufPtr != buf)  free(bufPtr);

		return 0;
	}

	for (i=0; i<numShaders; ++i)
	{
		// after linking, they don't need to be attached anymore.
		// no idea  why they even are, if they don't have to..
		glDetachShader(shaderProgram, shaders[i]);
	}

	return shaderProgram;
}

#define MULTILINE_STRING(...) #__VA_ARGS__

// ############## shaders for 2D rendering (HUD, menus, console, videos, ..) #####################

static const char* vertexSrc2D = MULTILINE_STRING(

		in vec2 position; // GL4_ATTRIB_POSITION
		in vec2 texCoord; // GL4_ATTRIB_TEXCOORD

		// for UBO shared between 2D shaders
		layout (std140) uniform uni2D
		{
			mat4 trans;
		};

		out vec2 passTexCoord;

		void main()
		{
			gl_Position = trans * vec4(position, 0.0, 1.0);
			passTexCoord = texCoord;
		}
);

static const char* fragmentSrc2D = MULTILINE_STRING(

		in vec2 passTexCoord;

		// for UBO shared between all shaders (incl. 2D)
		layout (std140) uniform uniCommon
		{
			float gamma;
			float intensity;
			float intensity2D; // for HUD, menu etc

			vec4 color;
		};

		uniform sampler2D tex;

		out vec4 outColor;

		void main()
		{
			vec4 texel = texture(tex, passTexCoord);
			// the gl1 renderer used glAlphaFunc(GL_GREATER, 0.666);
			// and glEnable(GL_ALPHA_TEST); for 2D rendering
			// this should do the same
			if (texel.a <= 0.666)
				discard;

			// apply gamma correction and intensity
			texel.rgb *= intensity2D;
			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* fragmentSrc2Dpostprocess = MULTILINE_STRING(
		in vec2 passTexCoord;

		// for UBO shared between all shaders (incl. 2D)
		// TODO: not needed here, remove?
		layout (std140) uniform uniCommon
		{
			float gamma;
			float intensity;
			float intensity2D; // for HUD, menu etc

			vec4 color;
		};

		uniform sampler2D tex;
		uniform vec4 v_blend;

		out vec4 outColor;

		void main()
		{
			// no gamma or intensity here, it has been applied before
			// (this is just for postprocessing)
			vec4 res = texture(tex, passTexCoord);
			// apply the v_blend, usually blended as a colored quad with:
			res.rgb = v_blend.a * v_blend.rgb + (1.0 - v_blend.a)*res.rgb;
			outColor =  res;
		}
);

static const char* fragmentSrc2DpostprocessWater = MULTILINE_STRING(
		in vec2 passTexCoord;

		// for UBO shared between all shaders (incl. 2D)
		// atsb: apparently needed despite not being used.
		layout (std140) uniform uniCommon
		{
			float gamma;
			float intensity;
			float intensity2D; // for HUD, menu etc

			vec4 color;
		};

		const float PI = 3.14159265358979323846;

		uniform sampler2D tex;

		uniform float time;
		uniform vec4 v_blend;

		out vec4 outColor;

		void main()
		{
			vec2 uv = passTexCoord;

			// warping based on ref_vk
			float sx = 1.0 - abs(0.5 - uv.x) * 2.0;
			float sy = 1.0 - abs(0.5 - uv.y) * 2.0;
			float xShift = 2.0 * time + uv.y * PI * 10.0;
			float yShift = 2.0 * time + uv.x * PI * 10.0;
			vec2 distortion = vec2(sin(xShift) * sx, sin(yShift) * sy) * 0.00666;

			uv += distortion;
			uv = clamp(uv, vec2(0.0, 0.0), vec2(1.0, 1.0));

			vec4 res = texture(tex, uv);

			// apply the v_blend, usually blended as a colored quad with:
			res.rgb = v_blend.a * v_blend.rgb + (1.0 - v_blend.a) * res.rgb;
			outColor =  res;
		}
);

// 2D color only rendering, GL4_Draw_Fill(), GL4_Draw_FadeScreen()
static const char* vertexSrc2Dcolor = MULTILINE_STRING(

		in vec2 position; // GL4_ATTRIB_POSITION

		// for UBO shared between 2D shaders
		layout (std140) uniform uni2D
		{
			mat4 trans;
		};

		void main()
		{
			gl_Position = trans * vec4(position, 0.0, 1.0);
		}
);

static const char* fragmentSrc2Dcolor = MULTILINE_STRING(

		// for UBO shared between all shaders (incl. 2D)
		layout (std140) uniform uniCommon
		{
			float gamma;
			float intensity;
			float intensity2D; // for HUD, menus etc

			vec4 color;
		};

		out vec4 outColor;

		void main()
		{
			vec3 col = color.rgb * intensity2D;
			outColor.rgb = pow(col, vec3(gamma));
			outColor.a = color.a;
		}
);

// ############## shaders for 3D rendering #####################

static const char* vertexCommon3D = MULTILINE_STRING(

		in vec3 position;   // GL4_ATTRIB_POSITION
		in vec2 texCoord;   // GL4_ATTRIB_TEXCOORD
		in vec2 lmTexCoord; // GL4_ATTRIB_LMTEXCOORD
		in vec4 vertColor;  // GL4_ATTRIB_COLOR
		in vec3 normal;     // GL4_ATTRIB_NORMAL
		in uint lightFlags; // GL4_ATTRIB_LIGHTFLAGS

		out vec2 passTexCoord;

		// for UBO shared between all 3D shaders
		layout (std140) uniform uni3D
		{
			mat4 transProjView;
			mat4 transModel;

			float sscroll; // for SURF_FLOWING
			float tscroll; // for SURF_FLOWING
			float time;
			float alpha;
			float overbrightbits;
			float particleFadeFactor;
			float lightScaleForTurb; // surfaces with SURF_DRAWTURB (water, lava) don't have lightmaps, use this instead
			float _pad_1; // AMDs legacy windows driver needs this, otherwise uni3D has wrong size, round up to 16 bytes?
		};
);

static const char* fragmentCommon3D = MULTILINE_STRING(

		in vec2 passTexCoord;

		out vec4 outColor;

		// for UBO shared between all shaders (incl. 2D)
		layout (std140) uniform uniCommon
		{
			float gamma; // this is 1.0/vid_gamma
			float intensity;
			float intensity2D; // for HUD, menus etc

			vec4 color; // really?
		};
		// for UBO shared between all 3D shaders
		layout (std140) uniform uni3D
		{
			mat4 transProjView;
			mat4 transModel;

			float sscroll; // for SURF_FLOWING
			float tscroll; // for SURF_FLOWING
			float time;
			float alpha;
			float overbrightbits;
			float particleFadeFactor;
			float lightScaleForTurb; // surfaces with SURF_DRAWTURB (water, lava) don't have lightmaps, use this instead
			float _pad_1; // AMDs legacy windows driver needs this, otherwise uni3D has wrong size, round up to 16 bytes?
		};
);

static const char* vertexSrc3D = MULTILINE_STRING(

		// it gets attributes and uniforms from vertexCommon3D

		void main()
		{
			passTexCoord = texCoord;
			gl_Position = transProjView * transModel * vec4(position, 1.0);
		}
);

static const char* vertexSrc3Dflow = MULTILINE_STRING(

		// it gets attributes and uniforms from vertexCommon3D

		void main()
		{
			passTexCoord = texCoord + vec2(sscroll, tscroll);
			gl_Position = transProjView * transModel * vec4(position, 1.0);
		}
);

static const char* vertexSrc3Dlm = MULTILINE_STRING(

		// it gets attributes and uniforms from vertexCommon3D

		out vec2 passLMcoord;
		out vec3 passWorldCoord;
		out vec3 passNormal;
		flat out uint passLightFlags;

		void main()
		{
			passTexCoord = texCoord;
			passLMcoord = lmTexCoord;
			vec4 worldCoord = transModel * vec4(position, 1.0);
			passWorldCoord = worldCoord.xyz;
			vec4 worldNormal = transModel * vec4(normal, 0.0f);
			passNormal = normalize(worldNormal.xyz);
			passLightFlags = lightFlags;

			gl_Position = transProjView * worldCoord;
		}
);

static const char* vertexSrc3DlmFlow = MULTILINE_STRING(

		// it gets attributes and uniforms from vertexCommon3D

		out vec2 passLMcoord;
		out vec3 passWorldCoord;
		out vec3 passNormal;
		flat out uint passLightFlags;

		void main()
		{
			passTexCoord = texCoord + vec2(sscroll, tscroll);
			passLMcoord = lmTexCoord;
			vec4 worldCoord = transModel * vec4(position, 1.0);
			passWorldCoord = worldCoord.xyz;
			vec4 worldNormal = transModel * vec4(normal, 0.0f);
			passNormal = normalize(worldNormal.xyz);
			passLightFlags = lightFlags;

			gl_Position = transProjView * worldCoord;
		}
);

static const char* fragmentSrc3D = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		uniform sampler2D tex;

		void main()
		{
			vec4 texel = texture(tex, passTexCoord);

			// apply intensity and gamma
			texel.rgb *= intensity;
			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a*alpha; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* fragmentSrc3Dwater = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		uniform sampler2D tex;

		void main()
		{
			vec2 tc = passTexCoord;
			tc.s += sin( passTexCoord.t*0.125 + time ) * 4.0;
			tc.s += sscroll;
			tc.t += sin( passTexCoord.s*0.125 + time ) * 4.0;
			tc.s += tscroll;
			tc *= 1.0/64.0; // do this last

			vec4 texel = texture(tex, tc);

			// apply intensity and gamma
			texel.rgb *= intensity * lightScaleForTurb;
			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a*alpha; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* fragmentSrc3Dlm = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		struct DynLight { // gl4UniDynLight in C
			vec3 lightOrigin;
			float _pad;
			//vec3 lightColor;
			//float lightIntensity;
			vec4 lightColor; // .a is intensity; this way it also works on OSX...
			// (otherwise lightIntensity always contained 1 there)
		};

		layout (std140) uniform uniLights
		{
			DynLight dynLights[32];
			uint numDynLights;
			uint _pad1; uint _pad2; uint _pad3; // FFS, AMD!
		};

		uniform sampler2D tex;

		uniform sampler2D lightmap0;
		uniform sampler2D lightmap1;
		uniform sampler2D lightmap2;
		uniform sampler2D lightmap3;

		uniform vec4 lmScales[4];

		in vec2 passLMcoord;
		in vec3 passWorldCoord;
		in vec3 passNormal;
		flat in uint passLightFlags;

		void main()
		{
			vec4 texel = texture(tex, passTexCoord);

			// apply intensity
			texel.rgb *= intensity;

			// apply lightmap
			vec4 lmTex = texture(lightmap0, passLMcoord) * lmScales[0];
			lmTex     += texture(lightmap1, passLMcoord) * lmScales[1];
			lmTex     += texture(lightmap2, passLMcoord) * lmScales[2];
			lmTex     += texture(lightmap3, passLMcoord) * lmScales[3];

			if (passLightFlags != 0u)
			{
				// TODO: or is hardcoding 32 better?
				for (uint i=0u; i<numDynLights; ++i)
				{
					// I made the following up, it's probably not too cool..
					// it basically checks if the light is on the right side of the surface
					// and, if it is, sets intensity according to distance between light and pixel on surface

					// dyn light number i does not affect this plane, just skip it
					if ((passLightFlags & (1u << i)) == 0u)  continue;

					float intens = dynLights[i].lightColor.a;

					vec3 lightToPos = dynLights[i].lightOrigin - passWorldCoord;
					float distLightToPos = length(lightToPos);
					float fact = max(0.0, intens - distLightToPos - 52.0);

					// move the light source a bit further above the surface
					// => helps if the lightsource is so close to the surface (e.g. grenades, rockets)
					//    that the dot product below would return 0
					// (light sources that are below the surface are filtered out by lightFlags)
					lightToPos += passNormal*32.0;

					// also factor in angle between light and point on surface
					fact *= max(0.0, dot(passNormal, normalize(lightToPos)));


					lmTex.rgb += dynLights[i].lightColor.rgb * fact * (1.0/256.0);
				}
			}

			lmTex.rgb *= overbrightbits;
			outColor = lmTex*texel;
			outColor.rgb = pow(outColor.rgb, vec3(gamma)); // apply gamma correction to result

			outColor.a = 1.0; // lightmaps aren't used with translucent surfaces
		}
);

static const char* fragmentSrc3DlmNoColor = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		struct DynLight { // gl4UniDynLight in C
			vec3 lightOrigin;
			float _pad;
			//vec3 lightColor;
			//float lightIntensity;
			vec4 lightColor; // .a is intensity; this way it also works on OSX...
			// (otherwise lightIntensity always contained 1 there)
		};

		layout (std140) uniform uniLights
		{
			DynLight dynLights[32];
			uint numDynLights;
			uint _pad1; uint _pad2; uint _pad3; // FFS, AMD!
		};

		uniform sampler2D tex;

		uniform sampler2D lightmap0;
		uniform sampler2D lightmap1;
		uniform sampler2D lightmap2;
		uniform sampler2D lightmap3;

		uniform vec4 lmScales[4];

		in vec2 passLMcoord;
		in vec3 passWorldCoord;
		in vec3 passNormal;
		flat in uint passLightFlags;

		void main()
		{
			vec4 texel = texture(tex, passTexCoord);

			// apply intensity
			texel.rgb *= intensity;

			// apply lightmap
			vec4 lmTex = texture(lightmap0, passLMcoord) * lmScales[0];
			lmTex     += texture(lightmap1, passLMcoord) * lmScales[1];
			lmTex     += texture(lightmap2, passLMcoord) * lmScales[2];
			lmTex     += texture(lightmap3, passLMcoord) * lmScales[3];

			if (passLightFlags != 0u)
			{
				// TODO: or is hardcoding 32 better?
				for (uint i=0u; i<numDynLights; ++i)
				{
					// I made the following up, it's probably not too cool..
					// it basically checks if the light is on the right side of the surface
					// and, if it is, sets intensity according to distance between light and pixel on surface

					// dyn light number i does not affect this plane, just skip it
					if ((passLightFlags & (1u << i)) == 0u)  continue;

					float intens = dynLights[i].lightColor.a;

					vec3 lightToPos = dynLights[i].lightOrigin - passWorldCoord;
					float distLightToPos = length(lightToPos);
					float fact = max(0.0, intens - distLightToPos - 52.0);

					// move the light source a bit further above the surface
					// => helps if the lightsource is so close to the surface (e.g. grenades, rockets)
					//    that the dot product below would return 0
					// (light sources that are below the surface are filtered out by lightFlags)
					lightToPos += passNormal*32.0;

					// also factor in angle between light and point on surface
					fact *= max(0.0, dot(passNormal, normalize(lightToPos)));


					lmTex.rgb += dynLights[i].lightColor.rgb * fact * (1.0/256.0);
				}
			}

			// turn lightcolor into grey for gl4_colorlight 0
			lmTex.rgb = vec3(0.333 * (lmTex.r+lmTex.g+lmTex.b));

			lmTex.rgb *= overbrightbits;
			outColor = lmTex*texel;
			outColor.rgb = pow(outColor.rgb, vec3(gamma)); // apply gamma correction to result

			outColor.a = 1; // lightmaps aren't used with translucent surfaces
		}
);

static const char* fragmentSrc3Dcolor = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		void main()
		{
			vec4 texel = color;

			// apply gamma correction and intensity
			// texel.rgb *= intensity; TODO: use intensity here? (this is used for beams)
			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a*alpha; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* fragmentSrc3Dsky = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		uniform sampler2D tex;

		void main()
		{
			vec4 texel = texture(tex, passTexCoord);

			// TODO: something about GL_BLEND vs GL_ALPHATEST etc

			// apply gamma correction
			// texel.rgb *= intensity; // TODO: really no intensity for sky?
			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a*alpha; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* fragmentSrc3Dsprite = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		uniform sampler2D tex;

		void main()
		{
			vec4 texel = texture(tex, passTexCoord);

			// apply gamma correction and intensity
			texel.rgb *= intensity;
			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a*alpha; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* fragmentSrc3DspriteAlpha = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		uniform sampler2D tex;

		void main()
		{
			vec4 texel = texture(tex, passTexCoord);

			if (texel.a <= 0.666)
				discard;

			// apply gamma correction and intensity
			texel.rgb *= intensity;
			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a*alpha; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* vertexSrc3Dwater = MULTILINE_STRING(

		// it gets attributes and uniforms from vertexCommon3D
		void main()
		{
			passTexCoord = texCoord;

			gl_Position = transProjView * transModel * vec4(position, 1.0);
		}
);

static const char* vertexSrcAlias = MULTILINE_STRING(

		// it gets attributes and uniforms from vertexCommon3D

		out vec4 passColor;

		void main()
		{
			passColor = vertColor*overbrightbits;
			passTexCoord = texCoord;
			gl_Position = transProjView* transModel * vec4(position, 1.0);
		}
);

static const char* fragmentSrcAlias = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		uniform sampler2D tex;

		in vec4 passColor;

		void main()
		{
			vec4 texel = texture(tex, passTexCoord);

			// apply gamma correction and intensity
			texel.rgb *= intensity;
			texel.a *= alpha; // is alpha even used here?
			texel *= min(vec4(1.5), passColor);

			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* fragmentSrcAliasColor = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		in vec4 passColor;

		void main()
		{
			vec4 texel = passColor;

			// apply gamma correction and intensity
			// texel.rgb *= intensity; // TODO: color-only rendering probably shouldn't use intensity?
			texel.a *= alpha; // is alpha even used here?
			outColor.rgb = pow(texel.rgb, vec3(gamma));
			outColor.a = texel.a; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* vertexSrcParticles = MULTILINE_STRING(

		// it gets attributes and uniforms from vertexCommon3D

		out vec4 passColor;

		void main()
		{
			passColor = vertColor;
			gl_Position = transProjView * transModel * vec4(position, 1.0);

			// abusing texCoord for pointSize, pointDist for particles
			float pointDist = texCoord.y*0.1; // with factor 0.1 it looks good.

			gl_PointSize = texCoord.x/pointDist;
		}
);

static const char* fragmentSrcParticles = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		in vec4 passColor;

		void main()
		{
			vec2 offsetFromCenter = 2.0*(gl_PointCoord - vec2(0.5, 0.5)); // normalize so offset is between 0 and 1 instead 0 and 0.5
			float distSquared = dot(offsetFromCenter, offsetFromCenter);
			if (distSquared > 1.0) // this makes sure the particle is round
				discard;

			vec4 texel = passColor;

			// apply gamma correction and intensity
			//texel.rgb *= intensity; TODO: intensity? Probably not?
			outColor.rgb = pow(texel.rgb, vec3(gamma));

			// I want the particles to fade out towards the edge, the following seems to look nice
			texel.a *= min(1.0, particleFadeFactor*(1.0 - distSquared));

			outColor.a = texel.a; // I think alpha shouldn't be modified by gamma and intensity
		}
);

static const char* fragmentSrcParticlesSquare = MULTILINE_STRING(

		// it gets attributes and uniforms from fragmentCommon3D

		in vec4 passColor;

		void main()
		{
			// outColor = passColor;
			// so far we didn't use gamma correction for square particles, but this way
			// uniCommon is referenced so hopefully Intels Ivy Bridge HD4000 GPU driver
			// for Windows stops shitting itself (see https://github.com/yquake2/yquake2/issues/391)
			outColor.rgb = pow(passColor.rgb, vec3(gamma));
			outColor.a = passColor.a;
		}
);


#undef MULTILINE_STRING

enum {
	GL4_BINDINGPOINT_UNICOMMON,
	GL4_BINDINGPOINT_UNI2D,
	GL4_BINDINGPOINT_UNI3D,
	GL4_BINDINGPOINT_UNILIGHTS
};

static qboolean
initShader2D(gl4ShaderInfo_t* shaderInfo, const char* vertSrc, const char* fragSrc)
{
	GLuint shaders2D[2] = {0};
	GLuint prog = 0;

	if (shaderInfo->shaderProgram != 0)
	{
		Com_Printf("WARNING: calling initShader2D for gl4ShaderInfo_t that already has a shaderProgram!\n");
		glDeleteProgram(shaderInfo->shaderProgram);
	}

	//shaderInfo->uniColor = shaderInfo->uniProjMatrix = shaderInfo->uniModelViewMatrix = -1;
	shaderInfo->shaderProgram = 0;
	shaderInfo->uniLmScalesOrTime = -1;
	shaderInfo->uniVblend = -1;

	shaders2D[0] = CompileShader(GL_VERTEX_SHADER, vertSrc, NULL);
	if (shaders2D[0] == 0)  return false;

	shaders2D[1] = CompileShader(GL_FRAGMENT_SHADER, fragSrc, NULL);
	if (shaders2D[1] == 0)
	{
		glDeleteShader(shaders2D[0]);
		return false;
	}

	prog = CreateShaderProgram(2, shaders2D);

	// I think the shaders aren't needed anymore once they're linked into the program
	glDeleteShader(shaders2D[0]);
	glDeleteShader(shaders2D[1]);

	if (prog == 0)
	{
		return false;
	}

	shaderInfo->shaderProgram = prog;
	GL4_UseProgram(prog);

	// Bind the buffer object to the uniform blocks
	GLuint blockIndex = glGetUniformBlockIndex(prog, "uniCommon");
	if (blockIndex != GL_INVALID_INDEX)
	{
		GLint blockSize;
		glGetActiveUniformBlockiv(prog, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
		if (blockSize != sizeof(gl4state.uniCommonData))
		{
			Com_Printf("WARNING: OpenGL driver disagrees with us about UBO size of 'uniCommon': %i vs %i\n",
					blockSize, (int)sizeof(gl4state.uniCommonData));

			goto err_cleanup;
		}

		glUniformBlockBinding(prog, blockIndex, GL4_BINDINGPOINT_UNICOMMON);
	}
	else
	{
		Com_Printf("WARNING: Couldn't find uniform block index 'uniCommon'\n");
		// TODO: clean up?
		return false;
	}
	blockIndex = glGetUniformBlockIndex(prog, "uni2D");
	if (blockIndex != GL_INVALID_INDEX)
	{
		GLint blockSize;
		glGetActiveUniformBlockiv(prog, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
		if (blockSize != sizeof(gl4state.uni2DData))
		{
			Com_Printf("WARNING: OpenGL driver disagrees with us about UBO size of 'uni2D'\n");
			goto err_cleanup;
		}

		glUniformBlockBinding(prog, blockIndex, GL4_BINDINGPOINT_UNI2D);
	}
	else
	{
		Com_Printf("WARNING: Couldn't find uniform block index 'uni2D'\n");
		goto err_cleanup;
	}

	shaderInfo->uniLmScalesOrTime = glGetUniformLocation(prog, "time");
	if (shaderInfo->uniLmScalesOrTime != -1)
	{
		glUniform1f(shaderInfo->uniLmScalesOrTime, 0.0f);
	}

	shaderInfo->uniVblend = glGetUniformLocation(prog, "v_blend");
	if (shaderInfo->uniVblend != -1)
	{
		glUniform4f(shaderInfo->uniVblend, 0, 0, 0, 0);
	}

	return true;

err_cleanup:

	glDeleteProgram(prog);

	return false;
}

static qboolean
initShader3D(gl4ShaderInfo_t* shaderInfo, const char* vertSrc, const char* fragSrc)
{
	GLuint shaders3D[2] = {0};
	GLuint prog = 0;
	int i=0;

	if (shaderInfo->shaderProgram != 0)
	{
		Com_Printf("WARNING: calling initShader3D for gl4ShaderInfo_t that already has a shaderProgram!\n");
		glDeleteProgram(shaderInfo->shaderProgram);
	}

	shaderInfo->shaderProgram = 0;
	shaderInfo->uniLmScalesOrTime = -1;
	shaderInfo->uniVblend = -1;

	shaders3D[0] = CompileShader(GL_VERTEX_SHADER, vertexCommon3D, vertSrc);
	if (shaders3D[0] == 0)  return false;

	shaders3D[1] = CompileShader(GL_FRAGMENT_SHADER, fragmentCommon3D, fragSrc);
	if (shaders3D[1] == 0)
	{
		glDeleteShader(shaders3D[0]);
		return false;
	}

	prog = CreateShaderProgram(2, shaders3D);

	if (prog == 0)
	{
		goto err_cleanup;
	}

	GL4_UseProgram(prog);

	// Bind the buffer object to the uniform blocks
	GLuint blockIndex = glGetUniformBlockIndex(prog, "uniCommon");
	if (blockIndex != GL_INVALID_INDEX)
	{
		GLint blockSize;
		glGetActiveUniformBlockiv(prog, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
		if (blockSize != sizeof(gl4state.uniCommonData))
		{
			Com_Printf("WARNING: OpenGL driver disagrees with us about UBO size of 'uniCommon'\n");

			goto err_cleanup;
		}

		glUniformBlockBinding(prog, blockIndex, GL4_BINDINGPOINT_UNICOMMON);
	}
	else
	{
		Com_Printf("WARNING: Couldn't find uniform block index 'uniCommon'\n");

		goto err_cleanup;
	}
	blockIndex = glGetUniformBlockIndex(prog, "uni3D");
	if (blockIndex != GL_INVALID_INDEX)
	{
		GLint blockSize;
		glGetActiveUniformBlockiv(prog, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
		if (blockSize != sizeof(gl4state.uni3DData))
		{
			Com_Printf("WARNING: OpenGL driver disagrees with us about UBO size of 'uni3D'\n");
			Com_Printf("         driver says %d, we expect %d\n", blockSize, (int)sizeof(gl4state.uni3DData));

			goto err_cleanup;
		}

		glUniformBlockBinding(prog, blockIndex, GL4_BINDINGPOINT_UNI3D);
	}
	else
	{
		Com_Printf("WARNING: Couldn't find uniform block index 'uni3D'\n");

		goto err_cleanup;
	}
	blockIndex = glGetUniformBlockIndex(prog, "uniLights");
	if (blockIndex != GL_INVALID_INDEX)
	{
		GLint blockSize;
		glGetActiveUniformBlockiv(prog, blockIndex, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSize);
		if (blockSize != sizeof(gl4state.uniLightsData))
		{
			Com_Printf("WARNING: OpenGL driver disagrees with us about UBO size of 'uniLights'\n");
			Com_Printf("         OpenGL says %d, we say %d\n", blockSize, (int)sizeof(gl4state.uniLightsData));

			goto err_cleanup;
		}

		glUniformBlockBinding(prog, blockIndex, GL4_BINDINGPOINT_UNILIGHTS);
	}
	// else: as uniLights is only used in the LM shaders, it's ok if it's missing

	// make sure texture is GL_TEXTURE0
	GLint texLoc = glGetUniformLocation(prog, "tex");
	if (texLoc != -1)
	{
		glUniform1i(texLoc, 0);
	}

	// ..  and the 4 lightmap texture use GL_TEXTURE1..4
	char lmName[10] = "lightmapX";
	for (i=0; i<4; ++i)
	{
		lmName[8] = '0'+i;
		GLint lmLoc = glGetUniformLocation(prog, lmName);
		if (lmLoc != -1)
		{
			glUniform1i(lmLoc, i+1); // lightmap0 belongs to GL_TEXTURE1, lightmap1 to GL_TEXTURE2 etc
		}
	}

	GLint lmScalesLoc = glGetUniformLocation(prog, "lmScales");
	shaderInfo->uniLmScalesOrTime = lmScalesLoc;
	if (lmScalesLoc != -1)
	{
		shaderInfo->lmScales[0] = HMM_Vec4(1.0f, 1.0f, 1.0f, 1.0f);

		for (i=1; i<4; ++i)  shaderInfo->lmScales[i] = HMM_Vec4(0.0f, 0.0f, 0.0f, 0.0f);

		glUniform4fv(lmScalesLoc, 4, shaderInfo->lmScales[0].Elements);
	}

	shaderInfo->shaderProgram = prog;

	// I think the shaders aren't needed anymore once they're linked into the program
	glDeleteShader(shaders3D[0]);
	glDeleteShader(shaders3D[1]);

	return true;

err_cleanup:

	glDeleteShader(shaders3D[0]);
	glDeleteShader(shaders3D[1]);

	if (prog != 0)  glDeleteProgram(prog);

	return false;
}

static void initUBOs(void)
{
	gl4state.uniCommonData.gamma = 1.0f/vid_gamma->value;
	gl4state.uniCommonData.intensity = gl4_intensity->value;
	gl4state.uniCommonData.intensity2D = gl4_intensity_2D->value;
	gl4state.uniCommonData.color = HMM_Vec4(1, 1, 1, 1);

	glGenBuffers(1, &gl4state.uniCommonUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, gl4state.uniCommonUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, GL4_BINDINGPOINT_UNICOMMON, gl4state.uniCommonUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(gl4state.uniCommonData), &gl4state.uniCommonData, GL_DYNAMIC_DRAW);

	// the matrix will be set to something more useful later, before being used
	gl4state.uni2DData.transMat4 = HMM_Mat4();

	glGenBuffers(1, &gl4state.uni2DUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, gl4state.uni2DUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, GL4_BINDINGPOINT_UNI2D, gl4state.uni2DUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(gl4state.uni2DData), &gl4state.uni2DData, GL_DYNAMIC_DRAW);

	// the matrices will be set to something more useful later, before being used
	gl4state.uni3DData.transProjViewMat4 = HMM_Mat4();
	gl4state.uni3DData.transModelMat4 = gl4_identityMat4;
	gl4state.uni3DData.sscroll = 0.0f;
	gl4state.uni3DData.tscroll = 0.0f;
	gl4state.uni3DData.time = 0.0f;
	gl4state.uni3DData.alpha = 1.0f;
	// gl4_overbrightbits 0 means "no scaling" which is equivalent to multiplying with 1
	gl4state.uni3DData.overbrightbits = (gl4_overbrightbits->value <= 0.0f) ? 1.0f : gl4_overbrightbits->value;
	gl4state.uni3DData.particleFadeFactor = gl4_particle_fade_factor->value;
	gl4state.uni3DData.lightScaleForTurb = 1.0f;

	glGenBuffers(1, &gl4state.uni3DUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, gl4state.uni3DUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, GL4_BINDINGPOINT_UNI3D, gl4state.uni3DUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(gl4state.uni3DData), &gl4state.uni3DData, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &gl4state.uniLightsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, gl4state.uniLightsUBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, GL4_BINDINGPOINT_UNILIGHTS, gl4state.uniLightsUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(gl4state.uniLightsData), &gl4state.uniLightsData, GL_DYNAMIC_DRAW);

	gl4state.currentUBO = gl4state.uniLightsUBO;
}

static qboolean createShaders(void)
{
	if (!initShader2D(&gl4state.si2D, vertexSrc2D, fragmentSrc2D))
	{
		Com_Printf("WARNING: Failed to create shader program for textured 2D rendering!\n");
		return false;
	}
	if (!initShader2D(&gl4state.si2Dcolor, vertexSrc2Dcolor, fragmentSrc2Dcolor))
	{
		Com_Printf("WARNING: Failed to create shader program for color-only 2D rendering!\n");
		return false;
	}

	if (!initShader2D(&gl4state.si2DpostProcess, vertexSrc2D, fragmentSrc2Dpostprocess))
	{
		Com_Printf("WARNING: Failed to create shader program to render framebuffer object!\n");
		return false;
	}
	if (!initShader2D(&gl4state.si2DpostProcessWater, vertexSrc2D, fragmentSrc2DpostprocessWater))
	{
		Com_Printf("WARNING: Failed to create shader program to render framebuffer object under water!\n");
		return false;
	}

	const char* lightmappedFrag = (gl4_colorlight->value == 0.0f)
	                               ? fragmentSrc3DlmNoColor : fragmentSrc3Dlm;

	if (!initShader3D(&gl4state.si3Dlm, vertexSrc3Dlm, lightmappedFrag))
	{
		Com_Printf("WARNING: Failed to create shader program for textured 3D rendering with lightmap!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3Dtrans, vertexSrc3D, fragmentSrc3D))
	{
		Com_Printf("WARNING: Failed to create shader program for rendering translucent 3D things!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3DcolorOnly, vertexSrc3D, fragmentSrc3Dcolor))
	{
		Com_Printf("WARNING: Failed to create shader program for flat-colored 3D rendering!\n");
		return false;
	}
	/*
	if (!initShader3D(&gl4state.si3Dlm, vertexSrc3Dlm, fragmentSrc3D))
	{
		Com_Printf("WARNING: Failed to create shader program for blending 3D lightmaps rendering!\n");
		return false;
	}
	*/
	if (!initShader3D(&gl4state.si3Dturb, vertexSrc3Dwater, fragmentSrc3Dwater))
	{
		Com_Printf("WARNING: Failed to create shader program for water rendering!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3DlmFlow, vertexSrc3DlmFlow, lightmappedFrag))
	{
		Com_Printf("WARNING: Failed to create shader program for scrolling textured 3D rendering with lightmap!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3DtransFlow, vertexSrc3Dflow, fragmentSrc3D))
	{
		Com_Printf("WARNING: Failed to create shader program for scrolling textured translucent 3D rendering!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3Dsky, vertexSrc3D, fragmentSrc3Dsky))
	{
		Com_Printf("WARNING: Failed to create shader program for sky rendering!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3Dsprite, vertexSrc3D, fragmentSrc3Dsprite))
	{
		Com_Printf("WARNING: Failed to create shader program for sprite rendering!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3DspriteAlpha, vertexSrc3D, fragmentSrc3DspriteAlpha))
	{
		Com_Printf("WARNING: Failed to create shader program for alpha-tested sprite rendering!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3Dalias, vertexSrcAlias, fragmentSrcAlias))
	{
		Com_Printf("WARNING: Failed to create shader program for rendering textured models!\n");
		return false;
	}
	if (!initShader3D(&gl4state.si3DaliasColor, vertexSrcAlias, fragmentSrcAliasColor))
	{
		Com_Printf("WARNING: Failed to create shader program for rendering flat-colored models!\n");
		return false;
	}

	const char* particleFrag = fragmentSrcParticles;
	if (gl4_particle_square->value != 0.0f)
	{
		particleFrag = fragmentSrcParticlesSquare;
	}

	if (!initShader3D(&gl4state.siParticle, vertexSrcParticles, particleFrag))
	{
		Com_Printf("WARNING: Failed to create shader program for rendering particles!\n");
		return false;
	}

	gl4state.currentShaderProgram = 0;

	return true;
}

qboolean GL4_InitShaders(void)
{
	initUBOs();

	return createShaders();
}

static void deleteShaders(void)
{
	const gl4ShaderInfo_t siZero = {0};
	for (gl4ShaderInfo_t* si = &gl4state.si2D; si <= &gl4state.siParticle; ++si)
	{
		if (si->shaderProgram != 0)  glDeleteProgram(si->shaderProgram);
		*si = siZero;
	}
}

void GL4_ShutdownShaders(void)
{
	deleteShaders();

	// let's (ab)use the fact that all 4 UBO handles are consecutive fields
	// of the gl4state struct
	glDeleteBuffers(4, &gl4state.uniCommonUBO);
	gl4state.uniCommonUBO = gl4state.uni2DUBO = gl4state.uni3DUBO = gl4state.uniLightsUBO = 0;
}

qboolean GL4_RecreateShaders(void)
{
	// delete and recreate the existing shaders (but not the UBOs)
	deleteShaders();
	return createShaders();
}

static inline void
updateUBO(GLuint ubo, GLsizeiptr size, void* data)
{
	if (gl4state.currentUBO != ubo)
	{
		gl4state.currentUBO = ubo;
		glBindBuffer(GL_UNIFORM_BUFFER, ubo);
	}

	/*
		atsb: faster in 4.6 and we can use glMapBufferRange to update the entire buffer at once from the beginning.
		we don't need to use glBindBufferRange and we can leave that alone for now.
	*/
	glBufferData(GL_UNIFORM_BUFFER, size, NULL, GL_STREAM_DRAW); // atsb: GL_STREAM_DRAW

	/*
		atsb: we use GL_MAP_WRITE_BIT here to ensure synchronisation between CPU/GPU
		and to prevent any possible data races in cases of sync loss.

		we don't use the persistent mapping feature yet as that would require
		a bit more work with how the buffers are created and mapped.
	*/
	GLvoid* ptr = glMapBufferRange(GL_UNIFORM_BUFFER, 0, size, GL_MAP_WRITE_BIT);
	memcpy(ptr, data, size);
	glUnmapBuffer(GL_UNIFORM_BUFFER);
}

void GL4_UpdateUBOCommon(void)
{
	updateUBO(gl4state.uniCommonUBO, sizeof(gl4state.uniCommonData), &gl4state.uniCommonData);
}

void GL4_UpdateUBO2D(void)
{
	updateUBO(gl4state.uni2DUBO, sizeof(gl4state.uni2DData), &gl4state.uni2DData);
}

void GL4_UpdateUBO3D(void)
{
	updateUBO(gl4state.uni3DUBO, sizeof(gl4state.uni3DData), &gl4state.uni3DData);
}

void GL4_UpdateUBOLights(void)
{
	updateUBO(gl4state.uniLightsUBO, sizeof(gl4state.uniLightsData), &gl4state.uniLightsData);
}
