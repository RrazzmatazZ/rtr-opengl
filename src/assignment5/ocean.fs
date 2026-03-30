#version 330 core

in vec3 WorldPos;
out vec4 FragColor;

void main() {
    //wave height factor value
    float heightFactor = clamp((WorldPos.y + 15.0) / 30.0, 0.0, 1.0);

    vec3 deepColor  = vec3(0.0, 0.05, 0.2);
    vec3 midColor   = vec3(0.0, 0.4, 0.6);
    vec3 crestColor = vec3(0.9, 0.95, 1.0);

    vec3 finalColor;

    // interpolate smoothly based on those 3 segments
    if (heightFactor < 10.0) {
        // deep to mid
        finalColor = mix(deepColor, midColor, heightFactor * 2.0);
    } else {
        // mid to crest
        finalColor = mix(midColor, crestColor, (heightFactor - 0.5) * 2.0);
    }

    FragColor = vec4(finalColor, 1.0);
}