#version 330 core
out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPosition;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 uBaseColor;

uniform float uNumSteps;      // 3 color uNumSteps[dark, normal, specular area]
uniform float uEdgeThreshold; // edge threshold(line weight decision)
uniform float powValue;

void main() {
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vWorldPosition);
    vec3 V = normalize(viewPos - vWorldPosition);

    // Diffuse banding
    float intensity = max(dot(N, L), 0.0);
    // normalize step to intensity
    float stepVal = floor(intensity * uNumSteps) / uNumSteps;
    vec3 color = uBaseColor * stepVal;

    // Outline
    float viewDot = dot(N, V);
    if (viewDot < uEdgeThreshold) {
        color = vec3(0.0); //set edge to black color
    }

    // Specular
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), powValue);
    if (spec > 0.6) {
        color += vec3(0.3);
    }

    FragColor = vec4(color, 1.0);
}