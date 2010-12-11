// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	Nathan Turner
//
//  GPL V2, see source/licence.txt
// ==============================================================

#version 110

varying vec3 normal, lightDir, eyeVec;

uniform vec3 teamColour;

uniform sampler2D baseTexture;
uniform sampler2D normalMap;

void main()
{
	// sample textures
	vec4 texDiffuseBase = texture2D(baseTexture, vec2(gl_TexCoord[0].st));
	vec3 texNormalMap = texture2D(normalMap, vec2(gl_TexCoord[0].st)).xyz;
	
	// diffuse shading
	vec3 n_shade = normalize(normal);
	float diffuseIntensity = max(dot(lightDir, n_shade), 0.0);
	
	// calculate intensity from texture normal
	vec3 n = normalize(texNormalMap * 2.0 - 1.0);
	float bump = max(dot(lightDir, n), 0.0);
	
	// interpolate the base and team colour according to the base alpha
	vec3 baseColour = mix(teamColour, texDiffuseBase.rgb, texDiffuseBase.a);
	
	// light components
	vec4 diffuse = gl_LightSource[0].diffuse * gl_FrontMaterial.diffuse * (bump * 0.8 + diffuseIntensity * 0.2);
	vec4 ambient = gl_LightSource[0].ambient * gl_FrontMaterial.ambient;
	
	// final colour
	gl_FragColor = vec4((diffuse.rgb + ambient.rgb) * baseColour, diffuse.a + ambient.a);
}