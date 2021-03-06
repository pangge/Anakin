/* Copyright (c) 2018 Anakin Authors, Inc. All Rights Reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0
   
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License. 
*/

#ifndef ANAKIN_SABER_FUNCS_FAKE_QUANTIZE_ABS_MAX_H
#define ANAKIN_SABER_FUNCS_FAKE_QUANTIZE_ABS_MAX_H

#include "saber/funcs/base.h"
#include "saber/funcs/impl/impl_base.h"
#include "saber/funcs/impl/impl_fake_quantize_abs_max.h"
#ifdef NVIDIA_GPU
#include "saber/funcs/impl/cuda/saber_fake_quantize_abs_max.h"
#endif

#ifdef USE_X86_PLACE
#include "saber/funcs/impl/x86/saber_fake_quantize_abs_max.h"
#endif
#ifdef USE_ARM_PLACE
//todo
#include "saber/funcs/impl/impl_fake_quantize_abs_max.h"
#endif
namespace anakin {
namespace saber {

template<typename TargetType, DataType OpDtype>
class FakeQuantizeAbsMax : public BaseFunc<
        TargetType,
        OpDtype,
        ImplBase,
        FakeQuantizeAbsMaxParam> {
public:
    using BaseFunc<
        TargetType,
        OpDtype,
        ImplBase,
        FakeQuantizeAbsMaxParam>::BaseFunc;

    FakeQuantizeAbsMax() = default;

    typedef Tensor<TargetType> InDataTensor;
    typedef Tensor<TargetType> OutDataTensor;
    typedef Tensor<TargetType> OpTensor;
    typedef FakeQuantizeAbsMaxParam<TargetType> Param_t;
    typedef std::vector<InDataTensor *> Input_v;
    typedef std::vector<OutDataTensor *> Output_v;
    typedef std::vector<Shape> Shape_v;

    virtual SaberStatus compute_output_shape(const Input_v& input, Output_v& output, \
        Param_t& param) override {
        SaberStatus status;
        CHECK_EQ(input.size(), 1);
       
        Shape output_shape = input[0]->valid_shape();
        output[0]->set_shape(output_shape);
        switch (param.bit_length) {
            case 8:
                output[0]->set_dtype(AK_INT8);
                break;
            case 16:
                output[0]->set_dtype(AK_INT16);
                break;
            default:
                LOG(FATAL) << "other bit length has not been supported";
           
        }

        return SaberSuccess;
    }

    virtual SaberStatus init_impl(ImplEnum implenum) override {
        switch (implenum) {
            case VENDER_IMPL:
                this->_impl.push_back(new VenderFakeQuantizeAbsMax <TargetType, OpDtype>);
                return SaberSuccess;

            case SABER_IMPL:
                this->_impl.push_back(new SaberFakeQuantizeAbsMax <TargetType, OpDtype>);
                return SaberSuccess;

            default:
                return SaberUnImplError;
        }
    }

private:

    virtual void pick_best_static() override {
        if (true) // some condition?
            this->_best_impl = this->_impl[0];
    }

    virtual void pick_best_specify(ImplEnum implenum) override {
        this->_best_impl = this->_impl[0];
    }

};

}

}

#endif //ANAKIN_SABER_FUNCS_FAKE_QUANTIZE_ABS_MAX_H
