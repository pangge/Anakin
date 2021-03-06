/* Copyright (c) 2016 Anakin Authors All Rights Reserve.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. */

#ifndef ANAKIN2_TEST_SABER_TEST_SABER_FUNC_TEST_ARM_H
#define ANAKIN2_TEST_SABER_TEST_SABER_FUNC_TEST_ARM_H

#include "utils/unit_test/aktest.h"
#include "utils/logger/logger.h"
#include <fstream>
#include <vector>

#include "saber/lite/core/context_lite.h"
#include "saber/lite/core/tensor_op_lite.h"
#include "saber/lite/funcs/timer_lite.h"

using namespace anakin::test;

int read_file(std::vector<float> &results, const char* file_name) {

    std::ifstream infile(file_name);
    if (!infile.good()) {
        LOG(ERROR) << "Cannot open " << file_name;
        return false;
    }
    LOG(INFO) << "found filename: " << file_name;
    std::string line;
    while (std::getline(infile, line)) {
        results.push_back((float)atof(line.c_str()));
    }
    return 0;
}

static int get_rand(int start, int end) {
    int i = rand();
    i = (i % (end - start)) + start;
    return i;
}

template  <typename type,  typename type2>
static void basic_gemm(int m, int n, int k, const type* a, const type* b, const type2* bias, type2* c, \
    type2 alpha, type2 beta, \
    bool trans_a = false, bool trans_b = false, bool flag_bias = false, bool flag_relu = false) {
//#pragma omp parallel for
    for (int i = 0; i < m; ++i) {
        type2 bias_data = (type2)0;
        if (flag_bias) {
            bias_data = bias[i];
        }
        for (int j = 0; j < n; ++j) {
            type2 sum = static_cast<type2>(0);
            for (int l = 0; l < k; ++l) {
                type av;
                type bv;
                if (trans_a) {
                    av = a[l * m + i];
                } else{
                    av = a[i * k + l];
                }
                if (trans_b) {
                    bv = b[j * k + l];
                } else {
                    bv = b[l * n + j];
                }
                sum += av * bv;
            }
            type2 tmp = alpha * sum + beta * c[i * n + j] + bias_data;
            if (flag_relu) {
                c[i * n + j] = tmp > (type2)0? tmp : (type2)0;
            } else {
                c[i * n + j] = tmp;
            }
        }
    }
}

template <typename Dtype>
static void fill_bias_relu(Dtype* tensor, const Dtype* bias, int channel, int channel_size, \
    bool flag_bias, bool flag_relu) {
    Dtype* data = tensor;
    for (int j = 0; j < channel; ++j) {
        Dtype bias_c = flag_bias? bias[j] : 0;
        for (int i = 0; i < channel_size; i++) {
            data[i] += bias_c;
            if (flag_relu) {
                data[i] = data[i] > 0 ? data[i] : 0.f;
            }
        }
        data += channel_size;
    }
}

template <typename Dtype>
static void do_relu(Dtype* tensor, int size) {
    for (int j = 0; j < size; ++j) {
        tensor[j] = tensor[j] > 0 ? tensor[j] : (Dtype)0;
    }
}

inline bool is_a_ge_zero_and_a_lt_b(int a, int b) {
    return static_cast<unsigned>(a) < static_cast<unsigned>(b);
}

template <typename Dtype>
static void col2im(const Dtype* data_col, const int channels,
            const int height, const int width, const int kernel_h, const int kernel_w,
            const int pad_h, const int pad_w,
            const int stride_h, const int stride_w,
            const int dilation_h, const int dilation_w,
            Dtype* data_im) {

    memset(data_im, 0, height * width * channels * sizeof(Dtype));
    const int output_h = (height + 2 * pad_h - (dilation_h * (kernel_h - 1) + 1)) / stride_h + 1;
    const int output_w = (width + 2 * pad_w - (dilation_w * (kernel_w - 1) + 1)) / stride_w + 1;
    const int channel_size = height * width;

    for (int channel = channels; channel--; data_im += channel_size) {
        for (int kernel_row = 0; kernel_row < kernel_h; kernel_row++) {
            for (int kernel_col = 0; kernel_col < kernel_w; kernel_col++) {
                int input_row = -pad_h + kernel_row * dilation_h;

                for (int output_rows = output_h; output_rows; output_rows--) {
                    if (!is_a_ge_zero_and_a_lt_b(input_row, height)) {
                        data_col += output_w;
                    } else {
                        int input_col = -pad_w + kernel_col * dilation_w;

                        for (int output_col = output_w; output_col; output_col--) {
                            if (is_a_ge_zero_and_a_lt_b(input_col, width)) {
                                data_im[input_row * width + input_col] += *data_col;
                            }
                            data_col++;
                            input_col += stride_w;
                        }
                    }
                    input_row += stride_h;
                }
            }
        }
    }
}

//! for float, dtype1 and type2 is float
//! for int8, dytpe1 is char, dtype2 is int
template <typename Dtype1, typename Dtype2>
void deconv_basic(const Dtype1* din, Dtype2* dout, \
                          int num, int chout, int hout, int wout, \
                          int chin, int hin, int win, \
                          const Dtype1* weights, const Dtype2* bias, \
                          int group, int kernel_w, int kernel_h, int stride_w, \
                          int stride_h, int dila_w, int dila_h, \
                          int pad_w, int pad_h, bool flag_bias, bool flag_relu) {


    int m = chout * kernel_w * kernel_h / group;
    int n = hin * win;
    int k = chin / group;

    if (chin != chout || group != chin) {
                CHECK_EQ(chin % group, 0) << "input channel or group size error";
                CHECK_EQ(chout % group, 0) << "output channel or group size error";
    }

    anakin::saber::lite::Tensor<anakin::saber::lite::CPU> workspace_tensor;
    anakin::saber::lite::Shape workspace_shape(1, 1, 1, group * m * n);
    workspace_tensor.re_alloc(workspace_shape, anakin::saber::AK_FLOAT);

    int group_size_in = win * hin * chin / group;
    int group_size_out = wout * hout * chout / group;
    int group_size_coldata = m * n;
    int group_size_weights = chin * chout * kernel_w * kernel_h / (group * group);
    bool flag_1x1s1p1 = (kernel_w == 1) && (kernel_h == 1) && (stride_h == 1) && \
                        (stride_w == 1) && (pad_w == 1) && (pad_h == 1) && \
                        (dila_w == 1) && (dila_h == 1);

    Dtype2* workspace_ptr = static_cast<Dtype2*>(workspace_tensor.mutable_data());

    for (int i = 0; i < num; ++i) {
        const Dtype1* din_batch = din + i * chin * hin * win;
        Dtype2* dout_batch = dout + i * chout * hout * wout;

        Dtype2* col_data = workspace_ptr;
        if (flag_1x1s1p1) {
            col_data = dout_batch;
        }
        for (int g = 0; g < group; ++g) {
            const Dtype1* din_group = din_batch + g * group_size_in;
            const Dtype1* weights_group = weights + g * group_size_weights;
            Dtype2* coldata_group = col_data + g * group_size_coldata;
            basic_gemm<Dtype1, Dtype2>(m, n, k, weights_group, din_group, nullptr, coldata_group, \
                (Dtype2)1, (Dtype2)0, true, false, false, (!flag_bias && flag_relu));
        }

        if (!flag_1x1s1p1) {
            col2im(col_data, chout, hout, wout, kernel_h, kernel_w, pad_h, pad_w, \
                stride_h, stride_w, dila_h, dila_w, dout_batch);
        }
        //! add bias
        if (flag_bias) {
            fill_bias_relu(dout_batch, bias, chout, wout * hout, flag_bias, flag_relu);
        }
    }
}

/**
 * \brief basic direct convolution function
 */
//! for float, dtype1 and type2 is float
//! for int8, dytpe1 is char, dtype2 is int
template <typename Dtype1, typename Dtype2>
static void conv_basic(const Dtype1* din, Dtype2* dout, \
                          int num, int chout, int hout, int wout, \
                          int chin, int hin, int win, \
                          const Dtype1* weights, const Dtype2* bias, \
                          int group, int kernel_w, int kernel_h, int stride_w, int stride_h, int dila_w, int dila_h, \
                          int pad_w, int pad_h, bool flag_bias, bool flag_relu) {

    Dtype2 beta = 0;
    auto src_data = din;
    auto dst_data_ref = dout;
    auto weights_data = weights;
    auto with_bias = flag_bias;
    auto bias_data = bias;

    int in_num = num;
    int out_channels = chout;
    int out_h = hout;
    int out_w = wout;

    int in_channel = chin;
    int in_h = hin;
    int in_w = win;
    int out_c_group = out_channels / group;
    int in_c_group = in_channel / group;

    for (int n = 0; n < in_num; ++n) {
#pragma omp parallel for collapse(4)
        for (int g = 0; g < group; ++g) {
            for (int oc = 0; oc < out_c_group; ++oc) {
                for (int oh = 0; oh < out_h; ++oh) {
                    for (int ow = 0; ow < out_w; ++ow) {
                        int out_idx = n * group * out_c_group * out_h * out_w + g * out_c_group * out_h * out_w
                                      + oc * out_h * out_w + oh * out_w + ow;
                        Dtype2 bias_d = with_bias ? (bias_data[g * out_c_group + oc]) : (Dtype2)0;
                        dst_data_ref[out_idx] = bias_d;// + dst_data_ref[out_idx] * beta;
                        for (int ic = 0; ic < in_c_group; ++ic) {
                            for (int kh = 0; kh < kernel_h; ++kh) {
                                for (int kw = 0; kw < kernel_w; ++kw) {
                                    int iw = ow * stride_w - pad_w + kw * (dila_w);
                                    int ih = oh * stride_h - pad_h + kh * (dila_h);
                                    if (iw < 0 || iw >= in_w) continue;
                                    if (ih < 0 || ih >= in_h) continue;

                                    int iidx = n * in_channel * in_h * in_w
                                               + g * in_c_group * in_h * in_w
                                               + ic * in_h * in_w
                                               + ih * in_w
                                               + iw;
                                    int widx = g * out_c_group * in_c_group * kernel_h * kernel_w
                                               + oc * in_c_group * kernel_h * kernel_w
                                               + ic * kernel_h * kernel_w
                                               + kh * kernel_w
                                               + kw;

                                    dst_data_ref[out_idx]
                                            += src_data[iidx]
                                               * weights_data[widx];
                                }
                            }
                        }
                        if (flag_relu) {
                            dst_data_ref[out_idx] = dst_data_ref[out_idx] > (Dtype2)0 ? dst_data_ref[out_idx] : (Dtype2)0;
                        }
                    }
                }
            }
        }
    }
}

template <typename dtype>
int count_diff(const dtype* src1, const dtype* src2, int size, double max_ratio, float tensor_scale) {
    double sum_abs1 = 0.0;
    double sum_abs2 = 0.0;
    for (int i = 0; i < size; ++i) {
        sum_abs1 += fabs(src1[i]);
        sum_abs2 += fabs(src2[i]);
    }
    double mean_abs1 = sum_abs1 / size;
    double mean_abs2 = sum_abs2 / size;
    double mean_val = (mean_abs1 + mean_abs2) / 2.0;
    if (max_ratio <= 0) {
        max_ratio = 0.1;
    }
    int count = 0;
    for (int i = 0; i < size; ++i) {
        double abs_diff = fabs(src1[i] - src2[i]);
        double ratio =  abs_diff / (fabs(src1[i] + src2[i]) + 1e-12);
        if (ratio > max_ratio && abs_diff > (tensor_scale + 1e-5f) && abs_diff > mean_val * 0.1f) {
            ++count;
        }
    }
    return count;
}

class TestSaberLite : public Test {
public:
    TestSaberLite() {}
    ~TestSaberLite() {}

protected:
    virtual void setup() {}
    virtual void teardown() {}

};

#endif //ANAKIN2_TEST_SABER_TEST_SABER_FUNC_TEST_ARM_H
