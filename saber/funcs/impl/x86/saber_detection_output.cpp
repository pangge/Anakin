#include "saber/funcs/impl/x86/saber_detection_output.h"
#include "saber/funcs/impl/detection_helper.h"
namespace anakin{

namespace saber{
template <typename dtype>
void permute_data(const int nthreads, const dtype* data, const int num_classes, 
    const int num_data, const int num_dim, dtype* new_data) {
    for (int index = 0; index < nthreads; ++index) {
        const int i = index % num_dim;
        const int c = (index / num_dim) % num_classes;
        const int d = (index / num_dim / num_classes) % num_data;
        const int n = index / num_dim / num_classes / num_data;
        const int new_index = ((n * num_classes + c) * num_data + d) * num_dim + i;
        new_data[new_index] = data[index];
    }
}

template <DataType OpDtype>
SaberStatus SaberDetectionOutput<X86, OpDtype>::dispatch(const std::vector<Tensor<X86> *>& inputs,
    std::vector<Tensor<X86> *>& outputs,
    DetectionOutputParam<X86>& param) {

    Tensor<X86>* t_loc = inputs[0];
    Tensor<X86>* t_conf = inputs[1];
    Tensor<X86>* t_prior = inputs[2];

    const dtype* loc_data = static_cast<const dtype*>(t_loc->data());
    const dtype* prior_data = static_cast<const dtype*>(t_prior->data());
    const int num = t_loc->num();

    // Decode predictions.
    dtype* bbox_data = static_cast<dtype*>(_bbox_preds.mutable_data());
    const int loc_count = _bbox_preds.valid_size();
    decode_bboxes<dtype>(loc_count, loc_data, prior_data, param.type, \
        param.variance_encode_in_target, _num_priors, param.share_location, \
        _num_loc_classes, param.background_id, bbox_data);
    // Retrieve all decoded location predictions.
    if (!param.share_location) {
        dtype * bbox_permute_data = static_cast<dtype*>(_bbox_permute.mutable_data());
        permute_data<dtype>(loc_count, bbox_data, _num_loc_classes, _num_priors,
                              4, bbox_permute_data);
    }
    // Retrieve all confidences.
    dtype* conf_permute_data = static_cast<dtype*>(_conf_permute.mutable_data());
    permute_data<dtype>(t_conf->valid_size(), static_cast<dtype*>(t_conf->data()), \
         this->_num_classes, _num_priors, 1, conf_permute_data);

    memcpy(_bbox_cpu_data, static_cast<dtype*>(_bbox_preds.data()), \
                _bbox_preds.valid_size() * sizeof(dtype));
    memcpy(_conf_cpu_data, static_cast<dtype*>(_conf_permute.data()), \
                _conf_permute.valid_size() * sizeof(dtype));

    std::vector<dtype> result;

    nms_detect(_bbox_cpu_data, _conf_cpu_data, result, num, this->_num_classes, _num_priors, param.background_id, \
        param.keep_top_k, param.nms_top_k, param.conf_thresh, param.nms_thresh, param.nms_eta, param.share_location);

    if (result.size() == 0) {
        result.resize(7);
        for (int i = 0; i < 7; ++i) {
            result[i] = (dtype)-1;
        }
        outputs[0]->reshape(Shape({1, 1, 1, 7}));
    } else {
        outputs[0]->reshape(Shape({1, 1, static_cast<int>(result.size() / 7), 7}));
    }

    memcpy(outputs[0]->mutable_data(), result.data(), \
                result.size() * sizeof(dtype));

    return SaberSuccess;
}

template class SaberDetectionOutput<X86, AK_FLOAT>;
} //namespace anakin

} //namespace anakin
