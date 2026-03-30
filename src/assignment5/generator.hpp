#ifndef OCEAN_WAVES_HPP
#define OCEAN_WAVES_HPP

#include <vector>
#include <cmath>
#include <random>

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
        
        // Partition the wavelength domain geometrically
        float lambda_multiplier = std::pow(maxWavelength / minWavelength, 1.0f / (numWaves - 1));
        float current_lambda = minWavelength;

        for (int i = 0; i < numWaves; ++i) {
            float k = 2.0f * M_PI / current_lambda;
            float omega = std::sqrt(g * k); //  w = sqrt(gk)
            
            // The simplified Pierson-Moskowitz spectrum calculation
            float alpha = 8.1e-3f;
            float beta = 0.74f;
            float omega_0 = g / windSpeed;
            float S_omega = (alpha * g * g / std::pow(omega, 5.0f)) * std::exp(-beta * std::pow(omega_0 / omega, 4.0f));
            
            // relationship between amplitude and omega value
            float amplitude = std::sqrt(S_omega) * 0.1f; // scaling factor based on real-world dimensions.

            // Hasselmann diffusion
            float windAngle = dis(gen) * 2.0f * M_PI; // Simplified as random directions,  should be distributed around the wind direction.

            
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