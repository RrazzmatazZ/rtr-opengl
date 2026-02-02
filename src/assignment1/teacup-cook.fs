#version 330 core
out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPosition;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 uBaseColor;

uniform float uRoughness; // m
uniform float uF0;        // Fresnel

#define PI 3.14159265

//Beckmann distribution
float Distribution(vec3 N, vec3 H, float m) {
    float NdotH = max(dot(N, H), 0.0001);
    float NdotH2 = NdotH * NdotH;
    float m2 = m * m;
    float exponent = (NdotH2 - 1.0) / (m2 * NdotH2);
    return (exp(exponent)) / (4.0 * m2 * NdotH2 * NdotH2);
}

float Geometry(vec3 N, vec3 H, vec3 V, vec3 L) {
    float NdotH = max(dot(N, H), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float VdotH = max(dot(V, H), 0.0001);

    float G1 = (2.0 * NdotH * NdotV) / VdotH;
    float G2 = (2.0 * NdotH * NdotL) / VdotH;
    return min(1.0, min(G1, G2));
}

//Fresnel Equation
vec3 Fresnel(float HdotV, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - HdotV, 5.0);
}

void main() {
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vWorldPosition);
    vec3 V = normalize(viewPos - vWorldPosition);
    vec3 H = normalize(L + V);

    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    float HdotV = max(dot(H, V), 0.0);

    vec3 ambient = 0.1 * uBaseColor;

    if (NdotL > 0.0) {
        float D = Distribution(N, H, uRoughness);
        float G = Geometry(N, H, V, L);
        vec3 F = Fresnel(HdotV, vec3(uF0));

        vec3 specular = (D * G * F) / (PI * NdotL * NdotV + 0.0001);
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;

        vec3 color = ambient + (kD * uBaseColor / PI + specular) * lightColor * NdotL * 2.0;
        FragColor = vec4(color, 1.0);
    } else {
        FragColor = vec4(ambient, 1.0);
    }
}