#version 330 core

uniform vec3 camPosition; // so we can compute the view vector
out vec4 FragColor; // the output color of this fragment

// light uniform variables
uniform vec4 ambientLightColor;
uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float lightRadius;

// material properties
uniform vec3 reflectionColor; // not needed?
uniform float roughness;
uniform float metalness; // not needed?
// legacy uniforms, not needed for PBR
uniform float ambientReflectance;
uniform float diffuseReflectance;
uniform float specularReflectance;
uniform float specularExponent;

// material textures
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_ambient1;		// unused
uniform sampler2D texture_specular1;	// unused
uniform samplerCube skybox;
uniform sampler2D shadowMap;

// @PHIJ -- 
uniform sampler2D texture_translucency1;
uniform sampler2D leafOpacity; // <-- this is a material texture for opacity. Potentially.

// 'in' variables to receive the interpolated Position and Normal from the vertex shader
in vec4 worldPos;
in vec3 worldNormal;
in vec3 worldTangent;
in vec2 textureCoordinates;

in vec4 lightPos;


vec3 GetNormalMap()
{
   //NEW! Normal map

   // Sample normal map
   vec3 normalMap = texture(texture_normal1, textureCoordinates).rgb;
   // Unpack from range [0, 1] to [-1 , 1]
   normalMap = normalMap * 2.0 - 1.0;

   // This time we are storing Z component in the texture, no need to compute it. Instead we normalize just in case
   normalMap = normalize(normalMap);

   // Create tangent space matrix
   vec3 N = normalize(worldNormal);
   vec3 B = normalize(cross(worldTangent, N)); // Orthogonal to both N and T
   vec3 T = cross(N, B); // Orthogonal to both N and B. Since N and B are normalized and orthogonal, T is already normalized

   
   mat3 TBN = mat3(T, B, N);

   // Transform normal map from tangent space to world space
   return TBN * normalMap;
}



void main()
{
	vec4 texColor = texture(texture_diffuse1, textureCoordinates);
	vec4 transluscencySample = texture(texture_translucency1, textureCoordinates);
	
	bool directional = lightRadius <= 0;
    vec3 L = normalize(lightPosition - (directional ? vec3(0.0f) : worldPos.xyz));

	//vec3 L = normalize(lightPosition - worldPos.xyz); //light direction
	vec3 N = GetNormalMap();



	float lamDiff = dot(L,N);

       if(gl_FrontFacing){
		lamDiff = -lamDiff;
		//N *= -1.0f;
		//T *= -1.0f;
	}

	vec3 directLight = max(lamDiff, 0) * texColor.rgb * lightColor;
	vec3 transluscentLight = max(-lamDiff,0) * (transluscencySample.rgb * 0.25f) * lightColor;

	if(texColor.a < 0.5f ){
		discard;
	}
	
	FragColor = vec4(directLight + transluscentLight, 1.0f);

}

