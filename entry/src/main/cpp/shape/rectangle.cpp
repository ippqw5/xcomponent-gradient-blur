/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "rectangle.h"
#include "log.h"
#include <string>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "util/stb_image.h"
namespace {
    std::string vs = R"(#version 320 es
    out vec3 ourColor;
    void main(void)
    {
        const vec4 vertex[] = vec4[] ( vec4(-1.0, -1.0, 0.5, 1.0),
                                       vec4( 1.0, -1.0, 0.5, 1.0),
                                       vec4(-1.0,  1.0, 0.5, 1.0),
                                       vec4( 1.0,  1.0, 0.5, 1.0) );
    
        gl_Position = vertex[gl_VertexID];
        ourColor = vec3(1.0, 0.0, 1.0);
    }
    )";

    std::string fs = R"(#version 320 es
precision highp int;
precision highp float;
precision highp sampler2D;

layout (binding = 0) uniform sampler2D input_image;

layout (location = 0) out vec4 color;

void main(void)
{
    ivec2 texSize = textureSize(input_image, 0);
    vec2 texCoord = gl_FragCoord.xy / vec2(texSize);
    color = texture(input_image, texCoord);

//     if (color.r == 0.0 && color.b == 0.0 && color.g == 0.0) {
//         color = vec4(1.0, 0.0, 0.0, 1.0); 
//     } else if (color.r < 0.0) {
//         color = vec4(1.0, 1.0, 0.0, 1.0); 
//     } else if (color.g < 0.0) {
//         color = vec4(0.0, 1.0, 1.0, 1.0); 
//     } else if (color.b < 0.0) {
//         color = vec4(1.0, 0.0, 1.0, 1.0); 
//     }
//     return;



    float m = 1.0 + gl_FragCoord.y / 30.0;
    vec2 s = 1.0 / vec2(texSize);

    vec2 C = gl_FragCoord.xy;
    float scale = 1.0;
    vec2 P0 = C * scale + vec2(-m, -m);
    vec2 P1 = C * scale + vec2(-m, m);
    vec2 P2 = C * scale + vec2(m, -m);
    vec2 P3 = C * scale + vec2(m, m);

    P0 *= s;
    P1 *= s;
    P2 *= s;
    P3 *= s;

    vec4 a = textureLod(input_image, P0, 0.0);
    vec4 b = textureLod(input_image, P1, 0.0);
    vec4 c = textureLod(input_image, P2, 0.0);
    vec4 d = textureLod(input_image, P3, 0.0);


    vec4 f = a - b - c + d;

    m *= 2.0;
    color = vec4(f) / float(m * m);
//         if (color.r < -0.01)
//         color = vec4(1.0, 0.0, 0.0, 1.0);
}

    )";

    std::string cs_prefixsum2d_bygroup =
        R"(#version 320 es

precision highp int;
precision highp float;
precision highp image2D;

layout (local_size_x = 256) in;

layout (location = 0) uniform uint group_id;

layout (binding = 0, rgba16f) readonly uniform image2D input_image;
layout (binding = 1, rgba16f) writeonly uniform image2D output_image;

shared vec4 shared_data[gl_WorkGroupSize.x * uint(2)];

void main(void)
{
    uint id = gl_LocalInvocationID.x;
    uint rd_id;
    uint wr_id;
    uint mask;

    uint local_idx = id * uint(2);
    uint offset = gl_WorkGroupSize.x * 2u * group_id;
    ivec2 P0 = ivec2(local_idx + offset, gl_WorkGroupID.x);
    ivec2 P1 = ivec2(local_idx + uint(1) + offset, gl_WorkGroupID.x);

    const uint steps = uint(log2(float(gl_WorkGroupSize.x))) + 1u;
    uint step = 0u;

    shared_data[local_idx] = imageLoad(input_image, P0);
    shared_data[local_idx + uint(1)] = imageLoad(input_image, P1);

    barrier();
    
    for (step = 0u; step < steps; step++)
    {
        mask = (1u << step) - 1u;
        rd_id = ((id >> step) << (step + 1u)) + mask;
        wr_id = rd_id + 1u + (id & mask);

        shared_data[wr_id] += shared_data[rd_id];

        barrier();
    }
    
    imageStore(output_image, P0.yx, shared_data[local_idx]);
    imageStore(output_image, P1.yx, shared_data[local_idx + uint(1)]);
}
)";

    std::string cs_prefixsum2d_groupadd =
        R"(#version 320 es
precision highp int;
precision highp float;

layout (local_size_x = 256) in;

layout (binding = 0, rgba16f) uniform highp readonly image2D input_image;
layout (binding = 0, rgba16f) uniform highp writeonly image2D output_image;

layout (location = 0) uniform uint rd_group_id;
layout (location = 1) uniform uint wr_group_id;

shared vec4 shared_data[gl_WorkGroupSize.x * 2u];

void main(void)
{
    uint id = gl_LocalInvocationID.x;
    uint offset = gl_WorkGroupSize.x * 2u * wr_group_id;

    uint local_idx = id * 2u;

    ivec2 P0 = ivec2(gl_WorkGroupID.x, local_idx + offset);
    ivec2 P1 = ivec2(gl_WorkGroupID.x, local_idx + 1u + offset);

    shared_data[local_idx] = imageLoad(input_image, P0);
    shared_data[local_idx + 1u] = imageLoad(input_image, P1);

    barrier();

    vec4 tmp = imageLoad(input_image, ivec2(gl_WorkGroupID.x,(rd_group_id + 1u) * gl_WorkGroupSize.x * 2u - 1u));
    
    imageStore(output_image, P0, shared_data[local_idx] + tmp);
    imageStore(output_image, P1, shared_data[local_idx + 1u] + tmp);
}
)";

} // namespace

inline void CheckGLESInfo() {
    GLint maxWorkGroupInvocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &maxWorkGroupInvocations);
    LOGI("GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: %{public}d", maxWorkGroupInvocations);

    GLint range[2];
    GLint precision;

    // 检查顶点着色器中的 highp 支持
    glGetShaderPrecisionFormat(GL_VERTEX_SHADER, GL_HIGH_FLOAT, range, &precision);
    LOGI("Vertex Shader highp precision: %{public}d", precision);

    // 检查片段着色器中的 highp 支持
    glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER, GL_HIGH_FLOAT, range, &precision);
    if (precision > 0) {
        LOGI("Fragment Shader highp precision: %{public}d", precision);
    } else {
        LOGE("Fragment Shader highp precision is not supported.");
    }

    glGetShaderPrecisionFormat(GL_COMPUTE_SHADER, GL_HIGH_FLOAT, range, &precision);
    LOGI("Compute Shader highp precision: %{public}d", precision);
    
    GLint num = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &num);
    LOGI("Max Texture Size: %{public}d", num);
}


int32_t Rectangle::Init()
{
    CheckGLESInfo();
    
    drawShader = new Shader(false, vs.c_str(), fs.c_str());
    prefix_sum_2d_prog = new Shader(false, cs_prefixsum2d_bygroup.c_str());
    prefix_sum_2d_group_add_prog = new Shader(false, cs_prefixsum2d_groupadd.c_str());
    
    images[0] = loadImage("/data/storage/el1/bundle/image.png", 4);
    
    image_size = 1024;
    group_num = std::ceil(image_size / NUM_ELEMENTS);
    for (int i = 1; i < 3; i++) {
        glGenTextures(1, &images[i]);
        glBindTexture(GL_TEXTURE_2D, images[i]);
        glTexParameteri(images[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(images[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(images[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(images[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, image_size, image_size);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    
    glGenVertexArrays(1, &vao);

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    DispatchByGroups(images[0], images[1]);
    DispatchByGroups(images[1], images[2]);

    return 0;
}

// 绘图转换
void Rectangle::Update()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
//    
//     DispatchByGroups(images[0], images[1]);
//     DispatchByGroups(images[1], images[2]);
    
    drawShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, images[2]);
    
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

int32_t Rectangle::Quit(void)
{
    glDeleteVertexArrays(1, &vao);
    glDeleteTextures(3, images);
    glDeleteProgram(drawShader->ID);
    glDeleteProgram(prefix_sum_2d_prog->ID);

    delete drawShader;
    drawShader = nullptr;
    
    delete prefix_sum_2d_prog;
    prefix_sum_2d_prog = nullptr;
    
    delete prefix_sum_2d_group_add_prog;
    prefix_sum_2d_group_add_prog = nullptr;
    
    LOGE("Rectangle Quit success.");
    return 0;
}

void Rectangle::Animate() {  } // K0.05

GLuint Rectangle::loadImage(const char *path, int c) {
    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    float *data = nullptr;
    data = stbi_loadf(path, &width, &height, &channels, c);
    if (!data) {
        LOGE("Failed to load texture: %{public}s", path);
        return 0;
    }
    
    LOGI("width: %d height: %d", width, height);
    
    if (0 < c && c <= 4)
        channels = c;

    GLenum internalFormat = 0, dataFormat = 0;
    if (channels == 4) {
        internalFormat = GL_RGBA16F;
        dataFormat = GL_RGBA;
    } else if (channels == 3) {
        internalFormat = GL_RGB16F;
        dataFormat = GL_RGB;
    } else if (channels == 2) {
        internalFormat = GL_RG16F;
        dataFormat = GL_RG;
    } else if (channels == 1) {
        internalFormat = GL_R16F;
        dataFormat = GL_RED;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    
    // 使用 glTexStorage2D 分配存储空间
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, width, height);

    // 使用 glTexSubImage2D 上传纹理数据
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    if (data)
        stbi_image_free(data);
    return textureID;
}