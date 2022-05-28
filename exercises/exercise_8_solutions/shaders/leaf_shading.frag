#version 330 core

uniform vec3 camPosition; // so we can compute the view vector
out vec4 FragColor; // the output color of this fragment

// light uniform variables
uniform vec4 ambientLightColor;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightRadius;

// material properties
uniform vec3 reflectionColor;
uniform float roughness;
uniform float metalness;
// legacy uniforms, not needed for PBR
uniform float ambientReflectance;
uniform float diffuseReflectance;
uniform float specularReflectance;
uniform float specularExponent;

// material textures
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_ambient1;
uniform sampler2D texture_specular1;
uniform samplerCube skybox;
uniform sampler2D shadowMap;

// @PHIJ -- 
uniform sampler2D leafTex;


// 'in' variables to receive the interpolated Position and Normal from the vertex shader
in vec4 worldPos;
in vec3 worldNormal;
in vec3 worldTangent;
in vec2 textureCoordinates;

// TODO 8.1 : Add an 'in' variable for vertex position in light space
in vec4 lightPos;

void main()
{
	FragColor = texture(leafTex, textureCoordinates);
}
