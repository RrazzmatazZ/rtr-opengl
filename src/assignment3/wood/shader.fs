#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN;
} fs_in;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal1;
};

uniform Material material;
uniform vec3 viewPos;
uniform vec3 lightPos;

uniform int mappingMode;
uniform float bumpScale;


float getHeight(vec2 uv) {
    return texture(material.texture_diffuse1, uv).r;
}

// Bump Mapping Logic
vec3 CalculateNormalFromHeight(vec2 uv) {
    float step = 1.0 / 1024.0;
    float scale = bumpScale;

    float h_center = getHeight(uv);
    float h_right  = getHeight(uv + vec2(step, 0.0));
    float h_top    = getHeight(uv + vec2(0.0, step));

    float dHdU = (h_right - h_center) * scale;
    float dHdV = (h_top - h_center) * scale;

    vec3 bumpNormal = normalize(vec3(-dHdU, -dHdV, 1.0));

    return bumpNormal;
}

void main()
{
    vec3 normal;

    //bump mapping
    if (mappingMode == 1) {
        vec3 tangentNormal = CalculateNormalFromHeight(fs_in.TexCoords);
        normal = normalize(fs_in.TBN * tangentNormal);
    }
    //normal mapping
    else if (mappingMode == 2){
        normal = texture(material.texture_normal1, fs_in.TexCoords).rgb;
        normal = normal * 2.0 - 1.0;
        normal = normalize(fs_in.TBN * normal);
    }
    //none
    else {
        normal = normalize(fs_in.TBN[2]);
    }


    vec3 color = texture(material.texture_diffuse1, fs_in.TexCoords).rgb;
    vec3 ambient = 0.1 * color;

    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;

    // Specular
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);


    vec3 specularMapColor = vec3(texture(material.texture_specular1, fs_in.TexCoords));
    if (length(specularMapColor) < 0.01) specularMapColor = vec3(0.3);

    vec3 specular = vec3(0.5) * spec * specularMapColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}