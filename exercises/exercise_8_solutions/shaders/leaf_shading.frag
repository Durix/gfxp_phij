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
uniform float specularReflectance;		//unused??
uniform float specularExponent;			//unused?

// material textures
uniform sampler2D texture_diffuse1;		// gl_tex1 - albedo coloring
uniform sampler2D texture_normal1;		// gl_tex2 - normal map (wo/ bumps)
uniform sampler2D texture_ambient1;		// unused - no ambient texture
uniform sampler2D texture_specular1;	// unused - no specular mapping.
uniform samplerCube skybox;				// gl_tex5 - Skybox cubemap
uniform sampler2D shadowMap;			// gl_tex6 - shadow map

// @PHIJ -- 
uniform sampler2D texture_translucency1; // gl_tex7 - scattering texture
uniform sampler2D texture_roughness1; // gl_tex8 - roughness texture

// 'in' variables to receive the interpolated Position and Normal from the vertex shader
in vec4 worldPos;
in vec3 worldNormal;
in vec3 worldTangent;
in vec2 textureCoordinates;

//in vec4 lightPos;

const float PI = 3.14159265359;


vec3 GetAmbientLighting(vec3 albedo, vec3 normal)
{
   // TODO 8.2 : Remove this line
   //vec3 ambient = ambientLightColor.rgb * albedo;

   // TODO 8.2 : Get the ambient color by sampling the skybox using the normal.
   vec3 ambient = textureLod(skybox, normal, 5.0f).rgb;

   // TODO 8.2 : Scale the light by the albedo, considering also that it gets reflected equally in all directions
   ambient *= albedo / PI;

   // Only apply ambient during the first light pass
   ambient *= ambientLightColor.a; 

   // NEW! Ambient occlusion (try disabling it to see how it affects the visual result)
   float ambientOcclusion = texture(texture_ambient1, textureCoordinates).r;
   ambient *= ambientOcclusion;

   return ambient;
}

vec3 GetEnvironmentLighting(vec3 N, vec3 V)
{
   //NEW! Environment reflection

   // Compute reflected light vector (R)
   vec3 R = reflect(-V, N);

   // Sample cubemap
   // HACK: We sample a different mipmap depending on the roughness. Rougher surface will have blurry reflection
   vec3 reflection = textureLod(skybox, R, roughness * 5.0f).rgb;

   // We packed the amount of reflection in ambientLightColor.a
   // Only apply reflection (and ambient) during the first light pass
   reflection *= ambientLightColor.a; 

   return reflection;
}

// Taken from pbr_shading.frag
vec3 GetNormalMap()
{
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

// Schlick approximation of the Fresnel term
vec3 FresnelSchlick(vec3 F0, float cosTheta)
{
   return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float a)
{
   float a2 = a*a;
   float NdotH = max(dot(N, H), 0.0);
   float NdotH2 = NdotH*NdotH;

   float num = a2;
   float denom = (NdotH2 * (a2 - 1.0) + 1.0);
   denom = PI * denom * denom;

   return num / denom;
}

float GeometrySchlickGGX(float cosAngle, float a)
{
   float a2 = a*a;

   float num = 2 * cosAngle;
   float denom = cosAngle + sqrt(a2 + (1 - a2)*cosAngle*cosAngle);

   return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float a)
{
   float NdotV = max(dot(N, V), 0.0);
   float NdotL = max(dot(N, L), 0.0);
   float ggx2  = GeometrySchlickGGX(NdotV, roughness);
   float ggx1  = GeometrySchlickGGX(NdotL, roughness);

   return ggx1 * ggx2;
}

vec3 GetCookTorranceSpecularLighting(vec3 N, vec3 L, vec3 V)
{
   vec3 H = normalize(L + V);

   // Remap alpha parameter to roughness^2
   float a = roughness * roughness;

   float D = DistributionGGX(N, H, a);
   float G = GeometrySmith(N, V, L, a);

   float cosI = max(dot(N, L), 0.0);
   float cosO = max(dot(N, V), 0.0);

   // Important! Notice that Fresnel term (F) is not here because we apply it later when mixing with diffuse
   float specular = (D * G) / (4.0f * cosO * cosI + 0.0001f);

   return vec3(specular);
}

// blinn-phong diffuse. - NOT PBR COMPLIANT - 
/*
vec3 GetLambertianDiffuseLighting(vec3 N, vec3 L, vec3 albedo)
{
   vec3 diffuse = diffuseReflectance * albedo;
   diffuse /= PI;

   return diffuse;
}
*/

// super-simple Labertian Diffuse.
float GetLambertianDiffuse(vec3 lightDir, vec3 normalMap){
    return dot(lightDir,normalMap);
}

/*
float lambertianDiffuseOnFace(vec3 lightDir, vec3 normalMap){
    // normal mapping / front/back-face check
    float lamDiff = dot(lightDir,normalMap);
       if(gl_FrontFacing){
		lamDiff = -lamDiff;
		//N *= -1.0f;
		//T *= -1.0f;
	}
    return lamDiff;
}
*/

float GetAttenuation(vec4 P)
{
   float distToLight = distance(lightPosition, P.xyz);
   float attenuation = 1.0f / (distToLight * distToLight);

   float falloff = smoothstep(lightRadius, lightRadius*0.5f, distToLight);

   return attenuation * falloff;
}

// -- No shadowmap!
/*
float GetShadow()
{
   // TODO 8.1 : Transform the position in light space to shadow map space: from range (-1, 1) to range (0, 1)
   vec3 shadowMapSpacePos = (lightPos.xyz / lightPos.w) * 0.5 + 0.5;

   // TODO 8.1 : Sample the shadow map texture using the XY components of the light in shadow map space
   float depth = texture(shadowMap, shadowMapSpacePos.xy).r;

   // TODO 8.1 : Compare the depth value obtained with the Z component of the light in shadow map space. Return 0 if depth is smaller or equal, 1 otherwise
   return depth + 0.01f <= clamp(shadowMapSpacePos.z, -1, 1) ? 0.0 : 1.0;
}
*/

void main()
{
    
    // Variable declarations.
	vec4 texColor = texture(texture_diffuse1, textureCoordinates); // albedo
    // Alpha discarding - no need to do work on non-rendered fragments.
	if(texColor.a < 0.5f ){
		discard;
	}
    vec4 transluscencySample = texture(texture_translucency1, textureCoordinates); // translusency texture, secondary albedo.
    vec3 N = GetNormalMap();
    vec4 P = worldPos;
	bool directional = lightRadius <= 0; // Checks if light is directional
    vec3 L = normalize(lightPosition - (directional ? vec3(0.0f) : worldPos.xyz)); // light direction
	vec3 V = normalize(camPosition - P.xyz);
    vec3 H = normalize(L + V);
    vec3 F0 = vec3(0.04f);
    vec3 F = FresnelSchlick(F0, max(dot(H, V), 0.0));
	//-----

    vec3 diffuse = GetLambertianDiffuse(L, N) * texColor.rgb;
    if(gl_FrontFacing){
    // invert for translusency.
		diffuse = -diffuse;
		//N *= -1.0f;
		//T *= -1.0f;
	}
    vec3 specular = GetCookTorranceSpecularLighting(N, L, V); //  does not have F apllied yet.
    vec3 ambient = GetAmbientLighting(texColor.rgb, N);


    vec3 lightRadiance = lightColor;
    float attenuation = directional ? 1.0f : GetAttenuation(P);
    lightRadiance *= attenuation;
    lightRadiance *= max(dot(N, L), 0.0);
    vec3 FAmbient = FresnelSchlick(F0, max(dot(N, V), 0.0));
    vec3 indirectLight = mix(GetAmbientLighting(texColor.rgb,N), GetEnvironmentLighting(N,V), FAmbient);


	//vec3 directLight = max(diffuse, 0) * texColor.rgb * lightColor;
    vec3 transluscentLight = max(-diffuse,0) * (transluscencySample.rgb * 0.25f) * lightColor;
    
	vec3 directLight = max(mix(diffuse, specular, F),0);
    directLight *= lightRadiance;
    // -- Not sure why this doesn't work.
    //vec3 transluscentLight = max(mix(-diffuse, specular, F) * transluscencySample.rgb * 0.25f,0);
    
    
    
    // final frag coloring.
	FragColor = vec4(directLight + transluscentLight, 1.0f);
}

