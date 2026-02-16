#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal1;
};

uniform Material material;
uniform vec3 viewPos;
uniform vec3 lightPos;

// --- Mapping Control ---
uniform int mappingMode;
uniform float bumpScale;

// --- Shading Control ---
uniform int shadingMode;
uniform float roughness;
uniform float metallic;
uniform float specularStrength;
uniform float shininess;

const float PI = 3.14159265359;

float getHeight(vec2 uv) {
    return texture(material.texture_diffuse1, uv).r;
}

vec3 CalculateNormalFromHeight(vec2 uv) {
    float step = 1.0 / 1024.0;
    float scale = bumpScale;

    float h_center = getHeight(uv);
    float h_right  = getHeight(uv + vec2(step, 0.0));
    float h_top    = getHeight(uv + vec2(0.0, step));

    float dHdU = (h_right - h_center) * scale;
    float dHdV = (h_top - h_center) * scale;

    vec3 bumpNormal = normalize(vec3(-dHdU, -dHdV, 1.0));
    return bumpNormal;
}

// ----------------------------------------------------------------------------
// Cook-Torrance PBR Functions
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / max(denom, 0.0000001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 CalculateCookTorrance(vec3 N, vec3 V, vec3 L, vec3 albedo, vec3 radiance) {
    vec3 H = normalize(V + L);

    // F0: surface reflection at zero incidence
    // Dielectric: 0.04, Metal: Albedo
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
    vec3 specular = numerator / denominator;

    // Energy conservation: kS + kD = 1.0
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);

    // Outgoing radiance Lo
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ----------------------------------------------------------------------------
// Blinn-Phong Function
// ----------------------------------------------------------------------------
vec3 CalculateBlinnPhong(vec3 N, vec3 V, vec3 L, vec3 albedo, vec3 specularColor, vec3 lightColor) {
    // Diffuse
    float diff = max(dot(L, N), 0.0);
    vec3 diffuse = diff * albedo * lightColor;

    // Specular (Blinn)
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = specularStrength * spec * specularColor * lightColor;

    return diffuse + specular;
}

void main()
{
    // 1. Resolve Normal
    vec3 normal;
    if (mappingMode == 1) { // Bump
        vec3 tangentNormal = CalculateNormalFromHeight(fs_in.TexCoords);
        normal = normalize(fs_in.TBN * tangentNormal);
    }
    else if (mappingMode == 2) { // Normal Map
        normal = texture(material.texture_normal1, fs_in.TexCoords).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(fs_in.TBN * normal);
    }
    else { // None
        normal = normalize(fs_in.TBN[2]);
    }

    vec3 albedo = texture(material.texture_diffuse1, fs_in.TexCoords).rgb;
    vec3 specularMap = vec3(texture(material.texture_specular1, fs_in.TexCoords).r);

    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 lightColor = vec3(1.0);

    vec3 resultColor;
    vec3 ambient = 0.03 * albedo;

    if (shadingMode == 0) {
        // Blinn-Phong
        ambient = 0.1 * albedo;
        resultColor = ambient + CalculateBlinnPhong(normal, viewDir, lightDir, albedo, specularMap, lightColor);
    }
    else {
        // Cook-Torrance
        vec3 Lo = CalculateCookTorrance(normal, viewDir, lightDir, albedo, lightColor * 2.0);
        resultColor = ambient + Lo;

        resultColor = resultColor / (resultColor + vec3(1.0));
        resultColor = pow(resultColor, vec3(1.0/2.2));
    }

    FragColor = vec4(resultColor, 1.0);
}