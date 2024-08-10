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

#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <cstdint>
#include "shader.h"
#include "base_shape.h"

#define NUM_ELEMENTS 512
class Rectangle : public BaseShape {
public:
    int32_t Init() override;
    void Update(void) override;
    int32_t Quit(void) override;
    void Animate(void) override;
    
protected:
    Shader *drawShader = nullptr;
    Shader *prefix_sum_2d_prog = nullptr;
    Shader *prefix_sum_2d_group_add_prog = nullptr;
    unsigned int vao;
    
    // 纹理
    GLuint images[3] = {0};

    uint32_t group_num = 1;
    uint32_t image_size;
private:
    static GLuint loadImage(const char *path, int c);

    void DispatchByGroups(GLuint input_image, GLuint output_image) {
        {
            prefix_sum_2d_prog->use();

            // 获取绑定点的位置
            GLint input_image_location = glGetUniformLocation(prefix_sum_2d_prog->ID, "input_image");
            GLint output_image_location = glGetUniformLocation(prefix_sum_2d_prog->ID, "output_image");

            // 设置绑定点
            glUniform1i(input_image_location, 0);
            glUniform1i(output_image_location, 1);

            // 绑定输入和输出图像
            glBindImageTexture(0, input_image, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
            glBindImageTexture(1, output_image, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);


            GLint group_id_location = glGetUniformLocation(prefix_sum_2d_prog->ID, "group_id");
            for (uint32_t group_id = 0; group_id < group_num; group_id++) {
                glUniform1ui(group_id_location, group_id);
                glDispatchCompute(image_size, 1, 1);
                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
//             return;
            prefix_sum_2d_group_add_prog->use();
            GLint image_location = glGetUniformLocation(prefix_sum_2d_group_add_prog->ID, "input_image");
            glUniform1i(image_location, 0);
            glBindImageTexture(0, output_image, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
            
            uint32_t steps = log2(group_num);
            for (uint32_t step = 0; step < steps; step++) {
                for (uint32_t id = 0; id < group_num / 2u; id++) {
                    uint32_t mask = (1u << step) - 1u;
                    uint32_t rd_group_id = ((id >> step) << (step + 1)) + mask;
                    uint32_t wr_group_id = rd_group_id + 1u + (id & mask);

                    glUniform1ui(glGetUniformLocation(prefix_sum_2d_group_add_prog->ID, "rd_group_id"), rd_group_id);
                    glUniform1ui(glGetUniformLocation(prefix_sum_2d_group_add_prog->ID, "wr_group_id"), wr_group_id);

                    glDispatchCompute(image_size, 1, 1);
                    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
                }
            }
        }
    }
};


#endif // RECTANGLE_H
