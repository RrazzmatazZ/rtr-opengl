#version 330 core
out vec4 FragColor;

in vec3 vFragPos;
flat in vec3 vNormal; // Use the flat qualifier to disable interpolation.

uniform vec3 lightPos;
uniform vec3 viewPos;

void main()
{
    vec3 objectColor = vec3(1.0, 0.0, 0.0);
    vec3 lightColor = vec3(1.0, 1.0, 1.0);

    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(lightPos - vFragPos);

    vec3 ambient = 0.15 * lightColor;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor;

    vec3 result = (ambient + diffuse) * objectColor + specular;
    FragColor = vec4(result, 1.0);
}