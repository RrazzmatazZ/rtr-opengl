#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN; // 接收 TBN 矩阵
} fs_in;

struct Material {
    sampler2D texture_diffuse1;
    sampler2D texture_specular1;
    sampler2D texture_normal1; // 对应我们 C++ 里加载的贴图
};

uniform Material material;
uniform vec3 viewPos;
// 这里硬编码一个光源位置以便演示，实际项目中应由 Uniform 传入
uniform vec3 lightPos = vec3(0.0, 5.0, 5.0);

void main()
{
    // --- 臣之核心逻辑：Normal Mapping ---
    // 1. 采样法线贴图 [0, 1]
    vec3 normal = texture(material.texture_normal1, fs_in.TexCoords).rgb;

    // 2. 将范围从 [0, 1] 映射回 [-1, 1]
    normal = normal * 2.0 - 1.0;

    // 3. 将切线空间的法线转换到世界空间
    // 此时的 normal 已经包含了物体表面的微小凹凸细节
    normal = normalize(fs_in.TBN * normal);
    // ----------------------------------

    // --- Blinn-Phong 光照计算 ---

    // 漫反射贴图颜色
    vec3 color = texture(material.texture_diffuse1, fs_in.TexCoords).rgb;

    // 1. 环境光 (Ambient)
    vec3 ambient = 0.1 * color;

    // 2. 漫反射 (Diffuse)
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
    float diff = max(dot(lightDir, normal), 0.0); // 使用新的扰动法线
    vec3 diffuse = diff * color;

    // 3. 镜面反射 (Specular)
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0); // 使用新的扰动法线

    // 采样高光贴图 (若没有则默认白色或降低强度)
    vec3 specularMapColor = vec3(texture(material.texture_specular1, fs_in.TexCoords));
    vec3 specular = vec3(0.5) * spec * specularMapColor;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}