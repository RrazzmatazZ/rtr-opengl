#version 330 core

in vec3 WorldPos;
in vec3 ViewDir;

out vec4 FragColor;

const int MAX_WAVES = 60;
uniform int u_ActiveWaves;

uniform vec4 u_Waves[MAX_WAVES];
uniform vec4 u_WaveParams[MAX_WAVES];
uniform float u_Time;
uniform vec3 u_CameraPos;
uniform vec3 u_SunDir;
uniform vec3 u_SunColor;

uniform samplerCube u_Skybox;

const float N_min = 1.0;
const float N_max = 2.5;

float computeWeight(float lambda, float L) {
    float x = clamp((lambda / L - N_min) / (N_max - N_min), 0.0, 1.0);
    return 3.0 * x * x - 2.0 * x * x * x;
}

void main() {
    float distanceToCam = length(u_CameraPos - WorldPos);

    float L = max(distanceToCam * 0.0015, 0.0001);

    vec3 dp_dx = vec3(1.0, 0.0, 0.0);
    vec3 dp_dz = vec3(0.0, 0.0, 1.0);

    float sigma_x = 0.0;
    float sigma_y = 0.0;

    int loopCount = min(u_ActiveWaves, MAX_WAVES);
    for (int i = 0; i < loopCount; ++i) {
        float k_x = u_Waves[i].x;
        float k_y = u_Waves[i].y;
        float h = u_Waves[i].z;
        float omega = u_Waves[i].w;
        float lambda = u_WaveParams[i].x;

        float phase = omega * u_Time - (k_x * WorldPos.x + k_y * WorldPos.z);
        float k_len = length(vec2(k_x, k_y));

        float w_n = computeWeight(lambda, L);
        float w_r = 1.0 - w_n;

        float kh = k_len * h;
        if (kh > 0.95) {
            h = 0.95 / k_len;
        }
        if (w_n > 0.001) {
            float cos_p = cos(phase);
            float sin_p = sin(phase);

            dp_dx.x -= w_n * (k_x * k_x / k_len) * h * cos_p;
            dp_dx.y += w_n * k_x * h * sin_p;
            dp_dx.z -= w_n * (k_x * k_y / k_len) * h * cos_p;

            dp_dz.x -= w_n * (k_x * k_y / k_len) * h * cos_p;
            dp_dz.y += w_n * k_y * h * sin_p;
            dp_dz.z -= w_n * (k_y * k_y / k_len) * h * cos_p;
        }

        //Eq 4
        if (w_r > 0.001) {
            float arg = 1.0 - k_len * k_len * w_r * w_r * h * h;
            float var_contrib = 1.0 - sqrt(max(arg, 0.0));
            sigma_x += (k_x * k_x / (k_len * k_len)) * var_contrib;
            sigma_y += (k_y * k_y / (k_len * k_len)) * var_contrib;
        }
    }

    vec3 Normal = normalize(cross(dp_dz, dp_dx));

    if (dot(Normal, ViewDir) < 0.0) {
        Normal -= 2.0 * dot(Normal, ViewDir) * ViewDir;
    }


    float R0 = 0.02;
    float fresnel_sky = R0 + (1.0 - R0) * pow(1.0 - max(dot(ViewDir, Normal), 0.0), 5.0);

    vec3 reflectDir = reflect(-ViewDir, Normal);
    float variance = max(sigma_x + sigma_y, 0.001);

    //#region todo sun light

    //#endregion todo sun light

    // check mipmap level by variance value
    float mipLevel = 3.0 * sqrt(variance);

    vec3 skyColor = textureLod(u_Skybox, reflectDir, mipLevel).rgb;
    skyColor = pow(skyColor, vec3(2.2));// sRGB to linear space

    float skyboxIntensity = 1.0;
    vec3 I_sky = skyColor * skyboxIntensity * fresnel_sky;

    vec3 waterBaseColor = vec3(0.005, 0.02, 0.04) * (1.0 - fresnel_sky);

    // current final color = sky reflection + water base color
    // todo add sun light refection color
    vec3 color = I_sky + waterBaseColor;

    // HDR color mapping and Gamma adjustance
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}