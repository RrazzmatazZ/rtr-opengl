#ifndef OCEAN_WAVES_HPP
#define OCEAN_WAVES_HPP

#include <vector>
#include <cmath>
#include <random>

// 【要点 3: Physical Wave Model】
// 使用 Pierson-Moskowitz 频谱和 Hasselmann 方向扩展来生成深水波浪参数
struct WaveParameter {
    float amplitude; // h_i
    float omega;     // w_i (angular frequency)
    float k_x;       // wave vector x
    float k_y;       // wave vector y
    float lambda;    // wavelength
};

class OceanWaveGenerator {
public:
    static std::vector<WaveParameter> generateWaves(int numWaves, float windSpeed, float minWavelength, float maxWavelength) {
        std::vector<WaveParameter> waves;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.0f, 1.0f);

        const float g = 9.81f; // Gravity
        
        // 使用等比数列划分波长，以覆盖从毫米级到百米级的波浪
        float lambda_multiplier = std::pow(maxWavelength / minWavelength, 1.0f / (numWaves - 1));
        float current_lambda = minWavelength;

        for (int i = 0; i < numWaves; ++i) {
            float k = 2.0f * M_PI / current_lambda;
            float omega = std::sqrt(g * k); //  w = sqrt(gk)
            
            // Pierson-Moskowitz 频谱简化计算
            float alpha = 8.1e-3f;
            float beta = 0.74f;
            float omega_0 = g / windSpeed;
            float S_omega = (alpha * g * g / std::pow(omega, 5.0f)) * std::exp(-beta * std::pow(omega_0 / omega, 4.0f));
            
            // 振幅正比于频谱的平方根
            float amplitude = std::sqrt(S_omega) * 0.1f; // 缩放因子基于具体世界尺度

            // 随机风向扩散 (Hasselmann)
            float windAngle = dis(gen) * 2.0f * M_PI; // 简化为随机方向，实际应围绕主风向分布
            
            waves.push_back({
                amplitude,
                omega,
                k * std::cos(windAngle),
                k * std::sin(windAngle),
                current_lambda
            });

            current_lambda *= lambda_multiplier;
        }
        return waves;
    }
};

#endif