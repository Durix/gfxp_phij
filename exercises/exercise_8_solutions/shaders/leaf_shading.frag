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
uniform float roughness; // - overwritten by roughness texture sample per fragment.
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
uniform sampler2D texture_specular1;	// unused - no specular texture, we compute this instead.
uniform samplerCube skybox;				// gl_tex5 - Skybox cubemap
uniform sampler2D shadowMap;			// gl_tex6 - shadow map

// @PHIJ -- 
uniform sampler2D texture_translucency1;// gl_tex7 - translucency texture
uniform sampler2D texture_roughness1;   // gl_tex8 - roughness texture
uniform float epsilonC; // - A float of a computed constant value for beer's law.

// 'in' variables to receive the interpolated Position and Normal from the vertex shader
in vec4 worldPos;
in vec3 worldNormal;
in vec3 worldTangent;
in vec2 textureCoordinates;

//in vec4 lightPos;

const float PI = 3.14159265359;
float rough_local; // replaced all uses of roughness uniform with roughness texture sampling.

vec3 GetAmbientLighting(vec3 albedo, vec3 normal)
{

   vec3 ambient = textureLod(skybox, normal, 5.0f).rgb;

   // Scale the light by the albedo, considering also that it gets reflected equally in all directions
   ambient *= albedo / PI;

   // Only apply ambient during the first light pass
   ambient *= ambientLightColor.a; 

   // Ambient occlusion
   float ambientOcclusion = texture(texture_ambient1, textureCoordinates).r;
   ambient *= ambientOcclusion;

   return ambient;
}

vec3 GetEnvironmentLighting(vec3 N, vec3 V)
{
   // Compute reflected light vector (R)
   vec3 R = reflect(-V, N);

   // Sample cubemap
   // HACK: We sample a different mipmap depending on the roughness. Rougher surface will have blurry reflection
   vec3 reflection = textureLod(skybox, R, rough_local * 5.0f).rgb;

   // We packed the amount of reflection in ambientLightColor.a
   // Only apply reflection (and ambient) during the first light pass
   reflection *= ambientLightColor.a; 

   return reflection;
}

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
   // invert the worldNormal for translucency on the backface. (AFTER calculating T)
   if(gl_FrontFacing){
        N *= -1.0f;
        //T *= -1.0f;
	    //normalMap.z = -normalMap.z;
    }
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

   float ggx2  = GeometrySchlickGGX(NdotV, rough_local);
   float ggx1  = GeometrySchlickGGX(NdotL, rough_local);


   return ggx1 * ggx2;
}

vec3 GetCookTorranceSpecularLighting(vec3 N, vec3 L, vec3 V)
{
   vec3 H = normalize(L + V);

   // Remap alpha parameter to rough_local^2
   float a = rough_local * rough_local;

   float D = DistributionGGX(N, H, a);
   float G = GeometrySmith(N, V, L, a);

   float cosI = max(dot(N, L), 0.0);
   float cosO = max(dot(N, V), 0.0);

   // Important! Notice that Fresnel term (F) is not here because we apply it later when mixing with diffuse
   float specular = (D * G) / (4.0f * cosO * cosI + 0.0001f);

   return vec3(specular);
}


// NOTE: not lambertian diffuse, we apply the dot(N,L) elsewhere. We only return the color based on being reflected in multiple directions.
vec3 GetLambertianDiffuse(vec3 texC){
    return texC / PI;
}

float GetAttenuation(vec4 P)
{
   float distToLight = distance(lightPosition, P.xyz);
   float attenuation = 1.0f / (distToLight * distToLight);

   float falloff = smoothstep(lightRadius, lightRadius*0.5f, distToLight);

   return attenuation * falloff;
}

void main()
{
    // Variable declarations.
	vec4 texColor = texture(texture_diffuse1, textureCoordinates); // albedo
    // Alpha discarding - no need to do work on non-rendered (transparent) fragments.
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
    vec3 F0 = vec3(0.028f); // Assumption that the leaf has the same base reflectance as human skin.
    vec3 F = FresnelSchlick(F0, max(dot(H, V), 0.0));
    // overwrite roughness uniform.
    rough_local = 1.0f - vec4(texture(texture_roughness1, textureCoordinates).rgb, 1.0f).r;

    //-----
    
    vec3 diffuse = GetLambertianDiffuse(texColor.rgb); // this is not diffuse lighting computed yet
    vec3 specular = GetCookTorranceSpecularLighting(N, L, V); //  does not have F apllied yet.
    vec3 ambient = GetAmbientLighting(texColor.rgb, N);


    vec3 lightRadiance = lightColor;
    float attenuation = directional ? 1.0f : GetAttenuation(P);
    lightRadiance *= attenuation;
    lightRadiance *= max(dot(N, L), 0.0f); // computes the lambertian diffuse lighting, not coloring.
    vec3 FAmbient = FresnelSchlick(F0, max(dot(N, V), 0.0));
    vec3 indirectLight = mix(ambient, GetEnvironmentLighting(N,V), FAmbient);
    
	//vec3 directLight = max(diffuse, 0) * texColor.rgb * lightColor;
    //vec3 transluscentLight = max(-diffuse,0) * (transluscencySample.rgb * 0.25f) * lightColor;
    
    /*
    notSpecular = mix(diffuse, translucent, exp(-ec*thickness));
    vec3 directLight = mix(notSpecular, specular, F);
    */
    

	//vec3 directLight = max(mix(diffuse, specular, F),0);
    //directLight *= lightRadiance;

    vec3 baseTranslucency = max(-dot(N,L), 0.0f) * texColor.rgb;
    float transSample = transluscencySample.r * 0.25f; // multiply by constant to reduce or increase saturation
    vec3 transluscentLight = baseTranslucency * transSample * lightColor;
    
    vec3 notSpecular = mix(diffuse, transluscentLight, exp(-epsilonC*(1-transSample) * 10.0f));
    vec3 directLight = mix(notSpecular, specular, F);
    directLight *= lightRadiance;
    // final frag coloring.
	//FragColor = vec4((indirectLight + directLight) + transluscentLight, 1.0f);
    
    FragColor = vec4((indirectLight + directLight), 1.0f);
    // specular * lightRadiance
}

