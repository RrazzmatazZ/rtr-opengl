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

// 针对高斯/Beckmann分布的 Lambda 遮蔽函数有理近似
float computeLambda(float a) {
    if (a >= 1.6) return 0.0;
    return (1.0 - 1.259 * a + 0.396 * a * a) / (3.535 * a + 2.181 * a * a);
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

        // Eq 4: Accumulated sub-pixel slope variance (LOD Effect)
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

    //#region Sun BRDF
    vec3 SL = normalize(u_SunDir);
    vec3 V = ViewDir;
    vec3 H = normalize(SL + V);

    float NdotV = max(dot(Normal, V), 0.001);
    float NdotL = max(dot(Normal, SL), 0.001);
    float NdotH = max(dot(Normal, H), 0.001);

    vec3 tangentX = normalize(dp_dx);
    vec3 tangentY = normalize(dp_dz);

    //#region 艺术化光轨控制 (Artistic Sun Trail Control)
    // 彻底恢复并改良你的方案：解绑宽度与长度的衰减关系！

    // 基础方差 (确保近处脚下的光斑足够大)
    float min_var = 0.02;
    float base_sx2 = max(sigma_x, min_var);
    float base_sy2 = max(sigma_y, min_var);

    // 1. 控制【收束力度】(Width)：只对横向方差 sx2 应用强距离衰减
    // 指数衰减比你原来的公式收缩得更坚决，数值 narrowRate 可以随意调大调小
    float narrowRate = 0.05; // 越大远处收得越细
    float distanceDecay = exp(-distanceToCam * narrowRate);
    float sx2 = base_sx2 * mix(0.0001, 5.0, distanceDecay); // 远处极限压缩到 0.1% 宽度

    // 2. 控制【光轨长度】(Length)：纵向方差 sy2 不受距离衰减，反而独立放大
    // 就是因为你之前把 sy2 也衰减了，光轨才会过早断掉
    float trailLength = 1.5; // 大于1拉长光轨，小于1缩短光轨
    float sy2 = base_sy2 * trailLength;

    // 3. 视线掠射角压榨（保留你的优秀思路，增强远处的锐利度）
    float grazingFactor = pow(NdotV, 0.4);
    sx2 *= mix(0.1, 1.0, grazingFactor);
    //#endregion

    // 1. 微表面斜率 (Microfacet slopes zeta_hx, zeta_hy)
    float z_hx = -dot(H, tangentX) / NdotH;
    float z_hy = -dot(H, tangentY) / NdotH;

    // 2. 高斯斜率概率分布 p(zeta_h)
    float exp_term = exp(-0.5 * ((z_hx * z_hx) / sx2 + (z_hy * z_hy) / sy2));
    float p_zeta = exp_term / (6.2831853 * sqrt(sx2 * sy2));

    // 3. 遮蔽函数的自变量 a_v, a_l
    float V_tx = dot(V, tangentX);
    float V_ty = dot(V, tangentY);
    float a_v = NdotV / sqrt(2.0 * max(sx2 * V_tx * V_tx + sy2 * V_ty * V_ty, 1e-7));

    float L_tx = dot(SL, tangentX);
    float L_ty = dot(SL, tangentY);
    float a_l = NdotL / sqrt(2.0 * max(sx2 * L_tx * L_tx + sy2 * L_ty * L_ty, 1e-7));

    // 计算 Smith 几何遮蔽 Lambda
    float lambda_v = computeLambda(a_v);
    float lambda_l = computeLambda(a_l);

    // 4. 菲涅尔项
    float VdotH = max(dot(V, H), 0.0);
    float fresnel_sun = R0 + (1.0 - R0) * pow(1.0 - VdotH, 5.0);

    // 5. 整合为公式 15
    float h_z4 = NdotH * NdotH * NdotH * NdotH;
    float denominator = 4.0 * h_z4 * NdotV * (1.0 + lambda_v + lambda_l);

    // 太阳光反射的最终辐射强度
    float specular_mag = (p_zeta * fresnel_sun) / max(denominator, 1e-7);

    vec3 I_sun = u_SunColor * specular_mag;
    //#endregion Sun BRDF

    // check mipmap level by variance value
    float variance = max(sigma_x + sigma_y, 0.001);
    float mipLevel = 3.0 * sqrt(variance);

    vec3 skyColor = textureLod(u_Skybox, reflectDir, mipLevel).rgb;
    skyColor = pow(skyColor, vec3(2.2)); // sRGB to linear space

    float skyboxIntensity = 1.0;
    vec3 I_sky = skyColor * skyboxIntensity * fresnel_sky;

    vec3 waterBaseColor = vec3(0.005, 0.02, 0.04) * (1.0 - fresnel_sky);

    // Final color = sky reflection + water base color + sun specular
    vec3 color = I_sky + waterBaseColor + I_sun;

    // HDR color mapping and Gamma adjustance
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}