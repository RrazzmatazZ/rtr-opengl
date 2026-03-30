#version 330 core

in vec3 WorldPos;
in vec3 ViewDir;

out vec4 FragColor;

const int NUM_WAVES = 60;
uniform vec4 u_Waves[NUM_WAVES];
uniform vec4 u_WaveParams[NUM_WAVES];
uniform float u_Time;
uniform vec3 u_CameraPos;
uniform vec3 u_SunDir;
uniform vec3 u_SunColor;

const float N_min = 1.0;
const float N_max = 2.5;

float computeWeight(float lambda, float L) {
    float x = clamp((lambda / L - N_min) / (N_max - N_min), 0.0, 1.0);
    return 3.0 * x * x - 2.0 * x * x * x;
}

void main() {
    float distanceToCam = length(u_CameraPos - WorldPos);
    float L = distanceToCam * 0.005;

    // x和z方向的切线偏导数，用于叉乘求法线 (Eq. 3)
    vec3 dp_dx = vec3(1.0, 0.0, 0.0);
    vec3 dp_dz = vec3(0.0, 0.0, 1.0);

    // 微表面坡度方差 (Eq. 4)
    float sigma_x_sq = 0.0;
    float sigma_y_sq = 0.0;

    // --- 步骤 1：几何细节的微观过滤 (LOD) ---
    for(int i = 0; i < NUM_WAVES; ++i) {
        float k_x = u_Waves[i].x;
        float k_y = u_Waves[i].y;
        float h = u_Waves[i].z;
        float omega = u_Waves[i].w;
        float lambda = u_WaveParams[i].x;

        float phase = omega * u_Time - (k_x * WorldPos.x + k_y * WorldPos.z);
        float k_len = length(vec2(k_x, k_y));

        float w_n = computeWeight(lambda, L); // 法线权重
        float w_r = 1.0 - w_n;                // 粗糙度(BRDF)权重

        // 【致命 Bug 修复区】：防止由于 main.cpp 中乘了 30.0f 导致法线崩溃
        float kh = k_len * h;
        // 如果波浪斜率 > 1，强制截断，否则波浪会卷曲，法线会翻转背向导致高光变成死白浴缸！
        if (kh > 0.99) { h = 0.99 / k_len; }

        // 累加平均法线的偏导数 (Eq. 3)
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

        // 累加坡度方差，用于 BRDF (Eq. 4)
        if (w_r > 0.001) {
            float arg = 1.0 - k_len * k_len * w_r * w_r * h * h;
            float var_contrib = 1.0 - sqrt(max(arg, 0.0));
            sigma_x_sq += (k_x * k_x / (k_len * k_len)) * var_contrib;
            sigma_y_sq += (k_y * k_y / (k_len * k_len)) * var_contrib;
        }
    }

    // 最终法线
    vec3 Normal = normalize(cross(dp_dz, dp_dx));

    // 修复由于过滤导致的背向法线 (论文第3.2节最后一句)
    if (dot(Normal, ViewDir) < 0.0) {
        Normal -= 2.0 * dot(Normal, ViewDir) * ViewDir;
    }

    // --- 步骤 2：Ross BRDF 高光计算 ---
    vec3 HalfVector = normalize(ViewDir + u_SunDir);
    float cos_theta_v = max(dot(ViewDir, Normal), 0.001);

    // 微表面坡度分布概率 p(zeta) (Eq. 7 简化版)
    float zeta_x = -(dot(HalfVector, vec3(1,0,0))) / max(HalfVector.y, 0.0001);
    float zeta_y = -(dot(HalfVector, vec3(0,0,1))) / max(HalfVector.y, 0.0001);

    sigma_x_sq = max(sigma_x_sq, 0.001);
    sigma_y_sq = max(sigma_y_sq, 0.001);

    float p_zeta = 1.0 / (2.0 * 3.14159 * sqrt(sigma_x_sq * sigma_y_sq)) * exp(-0.5 * (zeta_x*zeta_x / sigma_x_sq + zeta_y*zeta_y / sigma_y_sq));

    // Schlick 近似菲涅尔因子 (Eq. 14)
    float R0 = 0.02; // 水的固有反射率(大约2%)
    float F = R0 + (1.0 - R0) * pow(1.0 - max(dot(ViewDir, HalfVector), 0.0), 5.0);

    // 太阳高光反射 (Eq. 15)
    float denom = 4.0 * pow(HalfVector.y, 4.0) * cos_theta_v;
    vec3 I_sun = u_SunColor * p_zeta * F / max(denom, 0.001);

    // --- 步骤 3：天空环境反射 ---
    // （在主程序还没给 shader 绑定真实天空盒时，我们先用一套绝美的程序化算法代替）
    vec3 reflectDir = reflect(-ViewDir, Normal);
    vec3 skyZenithColor = vec3(0.05, 0.15, 0.4);
    vec3 skyHorizonColor = vec3(0.7, 0.85, 0.95);
    float skyBlend = clamp(reflectDir.y * 2.0, 0.0, 1.0);
    vec3 simulatedSkyColor = mix(skyHorizonColor, skyZenithColor, skyBlend);

    vec3 I_sky = simulatedSkyColor * F;

    // 水下散射底色
    vec3 waterBaseColor = vec3(0.0, 0.1, 0.2) * (1.0 - F);

    // 合并光照
    vec3 color = I_sun + I_sky + waterBaseColor;

    // HDR 色调映射 (Tone Mapping) - 防止高光过曝发白
    color = color / (color + vec3(1.0));
    // Gamma 校正
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}