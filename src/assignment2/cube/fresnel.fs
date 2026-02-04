#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{
    float IOR = 1.5;
    vec3 I = normalize(Position - cameraPos);
    vec3 N = normalize(Normal);

    float f0 = pow((1.0 - IOR) / (1.0 + IOR), 2.0);
    float fresnel = f0 + (1.0 - f0) * pow(1.0 - max(dot(-I, N), 0.0), 5.0);

    vec3 reflectDir = reflect(I, N);
    vec3 reflectionColor = texture(skybox, reflectDir).rgb;

    vec3 refractDir = refract(I, N, 1.0 / IOR);
    vec3 refractionColor = texture(skybox, refractDir).rgb;

    vec3 finalColor = mix(refractionColor, reflectionColor, fresnel);

    FragColor = vec4(finalColor, 1.0);
}