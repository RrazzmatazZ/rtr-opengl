#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 WorldPos;
out vec3 ViewDir;

const int NUM_WAVES = 60; // 对应您 cpp 中的前 60 个波浪
uniform vec4 u_Waves[NUM_WAVES];
uniform vec4 u_WaveParams[NUM_WAVES];

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 u_CameraPos;
uniform float u_Time;

const float N_min = 1.0;
const float N_max = 2.5;

float computeWeight(float lambda, float L) {
    float x = clamp((lambda / L - N_min) / (N_max - N_min), 0.0, 1.0);
    return 3.0 * x * x - 2.0 * x * x * x;
}

void main() {
    vec4 worldPosData = model * vec4(aPos, 1.0);
    vec3 p = worldPosData.xyz;

    float distanceToCam = length(u_CameraPos - p);
    float L = distanceToCam * 0.005;

    vec3 displacement = vec3(0.0);

    // 【几何置换】：Eq. 2
    for(int i = 0; i < NUM_WAVES; ++i) {
        float k_x = u_Waves[i].x;
        float k_y = u_Waves[i].y;
        float h = u_Waves[i].z;
        float omega = u_Waves[i].w;
        float lambda = u_WaveParams[i].x;

        float w_p = computeWeight(lambda, L);

        if (w_p > 0.001) {
            float phase = omega * u_Time - (k_x * p.x + k_y * p.z);
            float k_len = length(vec2(k_x, k_y));

            // 安全钳制：防止由于 C++ 端暴力放大振幅导致波浪自我交叉打结
            float h_safe = h;
            if (k_len * h_safe > 1.0) h_safe = 1.0 / k_len;

            displacement.x += w_p * (k_x / k_len) * h_safe * sin(phase);
            displacement.z += w_p * (k_y / k_len) * h_safe * sin(phase);
            displacement.y += w_p * h_safe * cos(phase);
        }
    }

    WorldPos = p + displacement;
    ViewDir = normalize(u_CameraPos - WorldPos);

    gl_Position = projection * view * vec4(WorldPos, 1.0);
}