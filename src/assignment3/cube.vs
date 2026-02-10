#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    mat3 TBN; // 切线空间矩阵
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    // --- 臣之核心逻辑：构建 TBN 矩阵 ---
    // 将法线、切线、副切线从模型空间转换到世界空间
    // 这里的 normal matrix 简化为 model 矩阵，若有非均匀缩放需使用 transpose(inverse(mat3(model)))
    vec3 T = normalize(vec3(model * vec4(aTangent,   0.0)));
    vec3 B = normalize(vec3(model * vec4(aBitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal,    0.0)));

    // 构建正交矩阵，用于将切线空间的向量转到世界空间
    vs_out.TBN = mat3(T, B, N);
    // --------------------------------

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}