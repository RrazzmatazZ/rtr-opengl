#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform float powValue;
uniform float ks;

void main()
{
    vec3 objectColor = vec3(1.0, 0.0, 0.0); //red
    vec3 lightColor = vec3(1.0, 1.0, 1.0);  //white

    // Ambient
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Blinn-Phong
    float specularStrength = 0.8;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), powValue); // pow value
    vec3 specular = ks * specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse) * objectColor + specular;

    FragColor = vec4(result, 1.0);
}