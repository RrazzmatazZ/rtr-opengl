#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;
uniform float refraction;

void main()
{
    vec3 I = normalize(Position - cameraPos);
    vec3 R = refract(I, normalize(Normal), refraction);
    FragColor = texture(skybox, R);
}