#include "framework/operators/match_matrix.h"

namespace anakin {

namespace ops {

#ifdef USE_CUDA
template<>
void MatchMatrix<NV, Precision::FP32>::operator()(
    OpContext<NV>& ctx,
    const std::vector<Tensor4dPtr<NV> >& ins,
    std::vector<Tensor4dPtr<NV> >& outs) {
    auto* impl =
        static_cast<MatchMatrixHelper<NV, Precision::FP32>*>(this->_helper);
    auto& param =
        static_cast<MatchMatrixHelper<NV, Precision::FP32>*>(this->_helper)->_param_match_matrix;
    impl->_funcs_match_matrix(ins, outs, param, ctx);
}
#endif

#ifdef USE_X86_PLACE
template<>
void MatchMatrix<X86, Precision::FP32>::operator()(
        OpContext<X86>& ctx,
        const std::vector<Tensor4dPtr<X86> >& ins,
        std::vector<Tensor4dPtr<X86> >& outs) {
    auto* impl =
            static_cast<MatchMatrixHelper<X86, Precision::FP32>*>(this->_helper);
    auto& param =
            static_cast<MatchMatrixHelper<X86, Precision::FP32>*>(this->_helper)->_param_match_matrix;
    impl->_funcs_match_matrix(ins, outs, param, ctx);
}
#endif

/// TODO ... specialization other type of operator


/// set helper
template<typename Ttype, Precision Ptype>
MatchMatrixHelper<Ttype, Ptype>::~MatchMatrixHelper() {
}

template<typename Ttype, Precision Ptype>
Status MatchMatrixHelper<Ttype, Ptype>::InitParam() {
    DLOG(WARNING) << "Parsing MatchMatrix op parameter.";
    auto dim_in = GET_PARAMETER(int, dim_in);
    auto dim_t = GET_PARAMETER(int, dim_t);
    auto linear_term = GET_PARAMETER(bool, linear_term);
    auto bias_term = GET_PARAMETER(bool, bias_term);
    using pblock_type = PBlock<Ttype>;
    auto weights = GET_PARAMETER(pblock_type, weight_1);

    MatchMatrixParam<Ttype> param_match_matrix(dim_in, dim_t,            linear_term, bias_term, &(weights.d_tensor()));
    _param_match_matrix = param_match_matrix;

    return Status::OK();
}

template<typename Ttype, Precision Ptype>
Status MatchMatrixHelper<Ttype, Ptype>::Init(OpContext<Ttype>& ctx,
        const std::vector<Tensor4dPtr<Ttype> >& ins,
        std::vector<Tensor4dPtr<Ttype> >& outs) {
    SABER_CHECK(_funcs_match_matrix.init(ins, outs, _param_match_matrix, SPECIFY, SABER_IMPL, ctx));
    return Status::OK();
}

template<typename Ttype, Precision Ptype>
Status MatchMatrixHelper<Ttype, Ptype>::InferShape(const
        std::vector<Tensor4dPtr<Ttype> >& ins,
        std::vector<Tensor4dPtr<Ttype> >& outs) {
    SABER_CHECK(_funcs_match_matrix.compute_output_shape(ins, outs, _param_match_matrix));
    return Status::OK();
}

#ifdef USE_CUDA
template class MatchMatrixHelper<NV, Precision::FP32>;
template class MatchMatrixHelper<NV, Precision::FP16>;
template class MatchMatrixHelper<NV, Precision::INT8>;
#endif
#ifdef USE_ARM_PLACE
template class MatchMatrixHelper<ARM, Precision::FP32>;
template class MatchMatrixHelper<ARM, Precision::FP16>;
template class MatchMatrixHelper<ARM, Precision::INT8>;
#endif
#ifdef USE_X86_PLACE
template class MatchMatrixHelper<X86, Precision::FP32>;
template class MatchMatrixHelper<X86, Precision::FP16>;
template class MatchMatrixHelper<X86, Precision::INT8>;
#endif
// register helper
#ifdef USE_CUDA
ANAKIN_REGISTER_OP_HELPER(MatchMatrix, MatchMatrixHelper, NV, Precision::FP32);
#endif
#ifdef USE_ARM_PLACE
ANAKIN_REGISTER_OP_HELPER(MatchMatrix, MatchMatrixHelper, ARM, Precision::FP32);
#endif
#ifdef USE_X86_PLACE
ANAKIN_REGISTER_OP_HELPER(MatchMatrix, MatchMatrixHelper, X86, Precision::FP32);
#endif
//! register op
ANAKIN_REGISTER_OP(MatchMatrix)
.Doc("MatchMatrix operator")
#ifdef USE_CUDA
.__alias__<NV, Precision::FP32>("match_matrix")
#endif
#ifdef USE_ARM_PLACE
.__alias__<ARM, Precision::FP32>("match_matrix")
#endif
#ifdef USE_X86_PLACE
.__alias__<X86, Precision::FP32>("match_matrix")
#endif
.num_in(2)
.num_out(1)
.Args<int>("dim_in", " dims of input embedding ")
.Args<int>("dim_t", " out channels")
.Args<bool>("linear_term", " linear term nouse for now")
.Args<bool>("bias_term", " bias term nouse for now ");

} /* namespace ops */

} /* namespace anakin */


