#include "framework/operators/fusion_ops/conv_3x3_batchnorm_scale_relu.h"

namespace anakin {

namespace ops {

#define INSTANCE_SASSCONVBATCHNORMSCALERELU(Ttype, Ptype) \
template<> \
void SassConvBatchnormScaleRelu<Ttype, Ptype>::operator()(\
    OpContext<Ttype>& ctx,\
    const std::vector<Tensor4dPtr<Ttype> >& ins,\
    std::vector<Tensor4dPtr<Ttype> >& outs) {\
    auto* impl = static_cast<SassConvBatchnormScaleReluHelper<Ttype, Ptype>*>\
                 (this->_helper);\
    auto& param = static_cast<SassConvBatchnormScaleReluHelper<Ttype, Ptype>*>\
                  (this->_helper)->_param_conv_batchnorm_scale_relu;\
    SABER_CHECK(impl->_funcs_conv_batchnorm_scale_relu(ins, outs, param, ctx));\
}

/// TODO ... specialization other type of operator


/// set helper
template<typename Ttype, Precision Ptype>
SassConvBatchnormScaleReluHelper<Ttype, Ptype>::~SassConvBatchnormScaleReluHelper() {
}

template<typename Ttype, Precision Ptype>
Status SassConvBatchnormScaleReluHelper<Ttype, Ptype>::InitParam() {
    DLOG(WARNING) << "Parsing SassConvBatchnormScaleRelu op parameter.";

    // get conv param
    auto group = GET_PARAMETER(int, group);
    auto bias_term = GET_PARAMETER(bool, bias_term);
    auto padding = GET_PARAMETER(PTuple<int>, padding);
    auto strides = GET_PARAMETER(PTuple<int>, strides);
    auto dilation_rate = GET_PARAMETER(PTuple<int>, dilation_rate);
    auto filter_num = GET_PARAMETER(int, filter_num);
    auto kernel_size = GET_PARAMETER(PTuple<int>, kernel_size);
    auto axis = GET_PARAMETER(int, axis);

	
	using pblock_type = PBlock<Ttype>;
    auto weights = GET_PARAMETER(pblock_type, weight_1);
    auto weights_shape = weights.shape();

    // get batchnorm param
    auto epsilon = GET_PARAMETER(float, batchnorm_0_epsilon);
    auto momentum = GET_PARAMETER(float, batchnorm_0_momentum);
    auto batch_norm_weight_1 = GET_PARAMETER(pblock_type, batchnorm_0_weight_1);
    auto batch_norm_weight_1_vector = batch_norm_weight_1.vector();
    auto batch_norm_weight_2 = GET_PARAMETER(pblock_type, batchnorm_0_weight_2);
    auto batch_norm_weight_2_vector = batch_norm_weight_2.vector();
    auto batch_norm_weight_3 = GET_PARAMETER(pblock_type, batchnorm_0_weight_3);
    auto batch_norm_weight_3_vector = batch_norm_weight_3.vector();

    // get scale param
    auto scale_num_axes = GET_PARAMETER(int, scale_0_num_axes);
    auto scale_bias_term = GET_PARAMETER(bool, scale_0_bias_term);
    auto scale_axis = GET_PARAMETER(int, scale_0_axis);
    auto scale_weight_1 = GET_PARAMETER(pblock_type, scale_0_weight_1);
    auto scale_weight_1_vector = scale_weight_1.vector();
    auto scale_weight_2 = GET_PARAMETER(pblock_type, scale_0_weight_2);
    auto  scale_weight_2_vector = scale_weight_2.vector();

    // get relu param
    auto alpha = GET_PARAMETER(float, relu_0_alpha);
    ActivationParam<Ttype> active_param(Active_relu, alpha); // TEMP

    // check if batchnorm parameters have been optimized 
    auto is_param_updated = CHECK_PARAMETER(is_param_updated);
    if(!is_param_updated) {
        SET_PARAMETER(is_param_updated, true, bool);

        if(bias_term) {
            auto bias = GET_PARAMETER(pblock_type, weight_2);
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                    update_weights<float, Ttype>, weights,bias,
                    weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                    true, batch_norm_weight_3_vector[0], epsilon,
                    batch_norm_weight_1_vector, batch_norm_weight_2_vector,
                    scale_weight_1_vector, scale_weight_2_vector,
                    scale_bias_term);

            saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                                               strides[0], strides[1],
                                               dilation_rate[0], dilation_rate[1],
                                               &(weights.d_tensor()), &(bias.d_tensor()),
                                               active_param);
            _param_conv_batchnorm_scale_relu = conv_param;
        } else {
            pblock_type* bias = new pblock_type();
            SET_PARAMETER(bias_term, true, bool); // set attr bias_term true
            SET_PARAMETER(weight_2, *bias, pblock_type); // gen new bias

            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                    update_weights<float, Ttype>, weights, *bias,
                    weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                    false, batch_norm_weight_3_vector[0], epsilon,
                    batch_norm_weight_1_vector, batch_norm_weight_2_vector,
                    scale_weight_1_vector, scale_weight_2_vector,
                    scale_bias_term);

            saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                    strides[0], strides[1], dilation_rate[0], dilation_rate[1],
                    &(weights.d_tensor()), &(bias->d_tensor()), active_param);
            _param_conv_batchnorm_scale_relu = conv_param;
        }
    } else {
        auto bias = GET_PARAMETER(pblock_type, weight_2);
        saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                strides[0], strides[1], dilation_rate[0], dilation_rate[1],
                &(weights.d_tensor()), &(bias.d_tensor()), active_param);
        _param_conv_batchnorm_scale_relu = conv_param;
    }

    return Status::OK();
}

template<typename Ttype, Precision Ptype>
Status SassConvBatchnormScaleReluHelper<Ttype, Ptype>::Init(OpContext<Ttype>& ctx,
        const std::vector<Tensor4dPtr<Ttype> >& ins,
        std::vector<Tensor4dPtr<Ttype> >& outs) {
    auto group = GET_PARAMETER(int, group);
    auto strides = GET_PARAMETER(PTuple<int>, strides);
    auto weights = GET_PARAMETER(PBlock<Ttype>, weight_1);
    auto bias_term = GET_PARAMETER(bool, bias_term);
    saber::ImplEnum impl_e = SABER_IMPL;

    SABER_CHECK(_funcs_conv_batchnorm_scale_relu.init(ins, outs,
            _param_conv_batchnorm_scale_relu, SPECIFY, impl_e, ctx));

    // check if weights have been transposed
    auto is_weights_transed = CHECK_PARAMETER(is_weights_transed);
    if (!is_weights_transed) {
        SET_PARAMETER(is_weights_transed, true, bool);

        if (bias_term) {
            auto bias = GET_PARAMETER(PBlock<Ttype>, weight_2);
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_1>(
                    std::bind(&Conv<Ttype, PrecisionWrapper<Ptype>::saber_type>::trans_weights,
                              &_funcs_conv_batchnorm_scale_relu, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                    weights.d_tensor(), bias.d_tensor(), _param_conv_batchnorm_scale_relu.pad_h, _param_conv_batchnorm_scale_relu.pad_w, _param_conv_batchnorm_scale_relu.dilation_h, _param_conv_batchnorm_scale_relu.dilation_w,
                    strides[0], strides[1], group, impl_e);
            bias.map_to_host();
        } else {
            PBlock<Ttype> bias_empty;
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_1>(
                    std::bind(&Conv<Ttype, PrecisionWrapper<Ptype>::saber_type>::trans_weights,
                              &_funcs_conv_batchnorm_scale_relu, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                    weights.d_tensor(), bias_empty.d_tensor(), _param_conv_batchnorm_scale_relu.pad_h, _param_conv_batchnorm_scale_relu.pad_w, _param_conv_batchnorm_scale_relu.dilation_h, _param_conv_batchnorm_scale_relu.dilation_w,
                    strides[0], strides[1], group, impl_e);
        }
        weights.map_to_host();
    } else {
        PBlock<Ttype> weight_empty;
        PBlock<Ttype> bias_empty;
        graph::GraphGlobalMem<Ttype>::Global().template apply<Level_1>(
                std::bind(&Conv<Ttype, PrecisionWrapper<Ptype>::saber_type>::trans_weights,
                          &_funcs_conv_batchnorm_scale_relu, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                weight_empty.d_tensor(), bias_empty.d_tensor(), _param_conv_batchnorm_scale_relu.pad_h, _param_conv_batchnorm_scale_relu.pad_w, _param_conv_batchnorm_scale_relu.dilation_h, _param_conv_batchnorm_scale_relu.dilation_w,
                strides[0], strides[1], group, impl_e);
    }
    return Status::OK();
}

//TODO!!! delete me when saber int8 is ready!!!!
#ifdef USE_CUDA
template<>
Status SassConvBatchnormScaleReluHelper<NV, Precision::INT8>::Init(OpContext<NV>& ctx,
        const std::vector<Tensor4dPtr<NV> >& ins,
        std::vector<Tensor4dPtr<NV> >& outs) {

    auto group = GET_PARAMETER(int, group);
    auto strides = GET_PARAMETER(PTuple<int>, strides);
    auto weights = GET_PARAMETER(PBlock<NV>, weight_1);
    auto bias_term = GET_PARAMETER(bool, bias_term);
    saber::ImplEnum impl_e = VENDER_IMPL;

    SABER_CHECK(_funcs_conv_batchnorm_scale_relu.init(ins, outs,
            _param_conv_batchnorm_scale_relu, SPECIFY, impl_e, ctx));

    // check if weights have been transposed
    auto is_weights_transed = CHECK_PARAMETER(is_weights_transed);
    if (!is_weights_transed) {
        SET_PARAMETER(is_weights_transed, true, bool);

        if (bias_term) {
            auto bias = GET_PARAMETER(PBlock<NV>, weight_2);
            graph::GraphGlobalMem<NV>::Global().template apply<Level_1>(
                    std::bind(&Conv<NV, PrecisionWrapper<Precision::INT8>::saber_type>::trans_weights,
                              &_funcs_conv_batchnorm_scale_relu, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                    weights.d_tensor(), bias.d_tensor(), _param_conv_batchnorm_scale_relu.pad_h, _param_conv_batchnorm_scale_relu.pad_w, _param_conv_batchnorm_scale_relu.dilation_h, _param_conv_batchnorm_scale_relu.dilation_w,
                    strides[0], strides[1], group, impl_e);
            bias.map_to_host();
        } else {
            PBlock<NV> bias_empty;
            graph::GraphGlobalMem<NV>::Global().template apply<Level_1>(
                    std::bind(&Conv<NV, PrecisionWrapper<Precision::INT8>::saber_type>::trans_weights,
                              &_funcs_conv_batchnorm_scale_relu, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                    weights.d_tensor(), bias_empty.d_tensor(), _param_conv_batchnorm_scale_relu.pad_h, _param_conv_batchnorm_scale_relu.pad_w, _param_conv_batchnorm_scale_relu.dilation_h, _param_conv_batchnorm_scale_relu.dilation_w,
                    strides[0], strides[1], group, impl_e);
        }
        weights.map_to_host();
    } else {
        PBlock<NV> weight_empty;
        PBlock<NV> bias_empty;
        graph::GraphGlobalMem<NV>::Global().template apply<Level_1>(
                std::bind(&Conv<NV, PrecisionWrapper<Precision::INT8>::saber_type>::trans_weights,
                          &_funcs_conv_batchnorm_scale_relu, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10),
                weight_empty.d_tensor(), bias_empty.d_tensor(), _param_conv_batchnorm_scale_relu.pad_h, _param_conv_batchnorm_scale_relu.pad_w, _param_conv_batchnorm_scale_relu.dilation_h, _param_conv_batchnorm_scale_relu.dilation_w,
                strides[0], strides[1], group, impl_e);
    }
    return Status::OK();
}
#endif
//TODO!!! end here

template<typename Ttype, Precision Ptype>
Status SassConvBatchnormScaleReluHelper<Ttype, Ptype>::InferShape(
    const std::vector<Tensor4dPtr<Ttype> >& ins,
    std::vector<Tensor4dPtr<Ttype> >& outs) {
    _funcs_conv_batchnorm_scale_relu.compute_output_shape(ins, outs,
            _param_conv_batchnorm_scale_relu);
    return Status::OK();
}

#ifdef USE_CUDA
template class SassConvBatchnormScaleReluHelper<NV, Precision::FP32>;
template class SassConvBatchnormScaleReluHelper<NV, Precision::FP16>;
template class SassConvBatchnormScaleReluHelper<NV, Precision::INT8>;
#endif

#ifdef USE_ARM_PLACE
template class SassConvBatchnormScaleReluHelper<ARM, Precision::FP32>;
template class SassConvBatchnormScaleReluHelper<ARM, Precision::FP16>;
template class SassConvBatchnormScaleReluHelper<ARM, Precision::INT8>;
#endif

// register helper
#ifdef USE_CUDA
INSTANCE_SASSCONVBATCHNORMSCALERELU(NV, Precision::FP32);
INSTANCE_SASSCONVBATCHNORMSCALERELU(NV, Precision::INT8);
ANAKIN_REGISTER_OP_HELPER(SassConvBatchnormScaleRelu, SassConvBatchnormScaleReluHelper, NV, Precision::FP32);
ANAKIN_REGISTER_OP_HELPER(SassConvBatchnormScaleRelu, SassConvBatchnormScaleReluHelper, NV, Precision::INT8);
#endif

#ifdef USE_X86_PLACE
INSTANCE_SASSCONVBATCHNORMSCALERELU(X86, Precision::FP32);
ANAKIN_REGISTER_OP_HELPER(SassConvBatchnormScaleRelu, SassConvBatchnormScaleReluHelper, X86, Precision::FP32);
#endif

#ifdef USE_ARM_PLACE
INSTANCE_SASSCONVBATCHNORMSCALERELU(ARM, Precision::FP32);
ANAKIN_REGISTER_OP_HELPER(SassConvBatchnormScaleRelu, SassConvBatchnormScaleReluHelper, ARM, Precision::FP32);
#endif

//! register op
ANAKIN_REGISTER_OP(SassConvBatchnormScaleRelu)
.Doc("SassConvBatchnormScaleRelu fusion operator")
#ifdef USE_CUDA
.__alias__<NV, Precision::FP32>("convolution3x3_batchnorm_scale_relu")
.__alias__<NV, Precision::INT8>("convolution3x3_batchnorm_scale_relu")
#endif
#ifdef USE_ARM_PLACE
.__alias__<ARM, Precision::FP32>("convolution_batchnorm_scale_relu")
#endif
.num_in(1)
.num_out(1)
.Args<int>("group", " group of conv ")
.Args<bool>("bias_term", " whether conv weights have bias")
.Args<PTuple<int>>("padding", "padding of conv (x, y)")
.Args<PTuple<int>>("strides", "strides of conv (x)")
.Args<PTuple<int>>("dilation_rate", "dilation rate of conv (x)")
.Args<int>("filter_num", "filter(kernel) number of weights")
.Args<PTuple<int>>("kernel_size", "kernel size of kernel (x, y)")
.Args<int>("axis", "axis of conv")
.Args<float>("relu_0_alpha", " alpha for relu")
.Args<int>("scale_0_num_axes", " num axes for scale")
.Args<bool>("scale_0_bias_term", "whether scale has bias")
.Args<int>("scale_0_axis", "axis for scale")
.Args<float>("batchnorm_0_epsilon", "epsilon for batchnorm")
.Args<float>("batchnorm_0_momentum", "momentum for batchnorm");

} /* namespace ops */

} /* namespace anakin */


