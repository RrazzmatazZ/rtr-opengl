#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

struct Material {
    sampler2D texture_diffuse1;
};

uniform Material material;

void main()
{
    vec3 color = texture(material.texture_diffuse1, TexCoords).rgb;

    color = pow(color, vec3(1.0/2.2));

    FragColor = vec4(color, 1.0);
}