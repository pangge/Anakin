#include "saber/funcs/impl/x86/saber_sequence_pool.h"
#include "saber/saber_funcs_param.h"
#include "saber/funcs/impl/x86/kernel/jit_generator.h"
#include <cstring>
#include <cmath>

namespace anakin{
namespace saber {

template <typename dtype>
void seq_pool_average(dtype* dst, const dtype* src_in,
                  const int slice_num, const int slice_size) {
    dtype sum = 0.f;
#pragma omp parallel for
    for (int i = 0; i < slice_size; ++i) {
        sum = src_in[i];
#pragma vector aligned
#pragma simd reduction(+:sum)
#pragma unroll(8)
        for (int s = 1; s < slice_num; ++s) {
            dtype src_in_read = src_in[s * slice_size +i];
            sum += src_in_read;
        }
        dst[i] = sum / slice_num;
    }
}

template <typename dtype>
void seq_pool_sum(dtype* dst, const dtype* src_in,
                  const int slice_num, const int slice_size) {
    dtype sum = 0.f;
#pragma omp parallel for
    for (int i = 0; i < slice_size; ++i) {
        sum = src_in[i];
#pragma vector aligned
#pragma simd reduction(+:sum)
#pragma unroll(8)
        for (int s = 1; s < slice_num; ++s) {
            dtype src_in_read = src_in[s * slice_size +i];
            sum += src_in_read;
        }
        dst[i] = sum;
    }
}

template <typename dtype>
void seq_pool_sqrt(dtype* dst, const dtype* src_in,
                  const int slice_num, const int slice_size) {
    dtype sqrt_len = sqrtf(slice_num);
    dtype sum = 0.f;
#pragma omp parallel for
    for (int i = 0; i < slice_size; ++i) {
        sum = src_in[i];
#pragma vector aligned
#pragma simd reduction(+:sum)
#pragma unroll(4)
        for (int s = 1; s < slice_num; ++s) {
            dtype src_in_read = src_in[s * slice_size +i];
            sum += src_in_read;
        }
        dst[i] = sum / sqrt_len;
    }
}

template <typename dtype>
void seq_pool_max(dtype* dst, const dtype* src_in,
                  const int slice_num, const int slice_size) {
    dtype max = 0.f;
#pragma omp parallel for
    for (int i = 0; i < slice_size; ++i) {
        max = src_in[i];
        for (int s = 1; s < slice_num; ++s) {
            dtype src_in_read = src_in[s * slice_size +i];
            if (max < src_in_read) {
                max = src_in_read;
            }
        }
        dst[i] = max;
    }
}

template <DataType OpDtype >
SaberStatus SaberSequencePool<X86, OpDtype>::init(
        const std::vector<DataTensor_in*>& inputs,
        std::vector<DataTensor_out*>& outputs,
        SequencePoolParam<X86> &param, Context<X86> &ctx) {

    this->_ctx = &ctx;
    kernel_direct_map = {
            {Sequence_pool_unknow, [](
                    DataType_in*, const DataType_in*, const int, const int){
                        LOG(ERROR) << " UNKNOWN seq pool type";}},

            {Sequence_pool_average, seq_pool_average<DataType_in>},
            {Sequence_pool_sum, seq_pool_sum<DataType_in>},
            {Sequence_pool_sqrt, seq_pool_sqrt<DataType_in>},
            {Sequence_pool_max, seq_pool_max<DataType_in>},

            {Sequence_pool_last, [](
                    DataType_in* dst, const DataType_in* src_in,
                    const int slice_num, const int slice_size) {
                memcpy(dst, src_in + slice_size * (slice_num - 1),
                       sizeof(DataType_in)* slice_size);
            }},
            {Sequence_pool_first, [](
                    DataType_in* dst, const DataType_in* src_in,
                    const int slice_num, const int slice_size) {
                memcpy(dst, src_in, sizeof(DataType_in)* slice_size);
            }},
    };
    return create(inputs, outputs, param, ctx);
}

template <DataType OpDtype>
SaberStatus SaberSequencePool<X86, OpDtype>::create(
        const std::vector<DataTensor_in*>& inputs,
        std::vector<DataTensor_out*>& outputs,
        SequencePoolParam<X86> &param,
        Context<X86> &ctx)
{

    this->_ctx = &ctx;

    return SaberSuccess;
}

template <DataType OpDtype>
SaberStatus SaberSequencePool<X86, OpDtype>::dispatch(
        const std::vector<DataTensor_in*>& inputs,
        std::vector<DataTensor_out*>& outputs,
        SequencePoolParam<X86> &param)
{

    CHECK_EQ(inputs[0]->channel(), outputs[0]->channel());
    CHECK_EQ(inputs[0]->height(), outputs[0]->height());
    CHECK_EQ(inputs[0]->width(), outputs[0]->width());

    std::vector<int> seq_offset = inputs[0]->get_seq_offset()[0];
    int slice_size = outputs[0]->channel()
                     * outputs[0]->height()
                     * outputs[0]->width();

    DataType_in* dst_ptr = (DataType_in*)outputs[0]->mutable_data();
    const DataType_out* src_ptr = (const DataType_out*)inputs[0]->data();
    for (int i = 0; i < seq_offset.size()-1; ++i) {
        int slice_num = seq_offset[i+1] - seq_offset[i];

        kernel_direct_map[param.sequence_pool_type](
                dst_ptr, src_ptr, slice_num, slice_size);

        dst_ptr += slice_size;
        src_ptr += slice_size * slice_num;
    }
    int batch_size=seq_offset.size()-1;
    std::vector<int> offset_new(batch_size+1);
    for(int i=0;i<=batch_size;++i){
        offset_new[i]=i;
    }
    std::vector<std::vector<int>> voffset_new;
    voffset_new.push_back(offset_new);
    outputs[0]->set_seq_offset(voffset_new);
    return SaberSuccess;

}
template class SaberSequencePool<X86, AK_FLOAT>;
DEFINE_OP_TEMPLATE(SaberSequencePool, SequencePoolParam, X86, AK_HALF);
DEFINE_OP_TEMPLATE(SaberSequencePool, SequencePoolParam, X86, AK_INT8);
}
} // namespace anakin