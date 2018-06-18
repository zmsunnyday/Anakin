/* Copyright (c) 2018 Baidu, Inc. All Rights Reserved.

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

#ifndef ANAKIN_SABER_FUNCS_CUDA_CUBLAS_FC_H
#define ANAKIN_SABER_FUNCS_CUDA_CUBLAS_FC_H

#include "saber/funcs/impl/impl_fc.h"

namespace anakin{

namespace saber{

template <DataType OpDtype,
        DataType inDtype,
        DataType outDtype,
        typename LayOutType_op,
        typename LayOutType_in,
        typename LayOutType_out>
class VenderFc<BM, OpDtype, inDtype, outDtype, LayOutType_op, LayOutType_in, LayOutType_out>: \
    public ImplBase<
        Tensor<BM, inDtype, LayOutType_in>, \
        Tensor<BM, outDtype, LayOutType_out>, \
        Tensor<BM, OpDtype, LayOutType_op>, \
        FcParam<Tensor<BM, OpDtype, LayOutType_op>>> {

public:
    typedef Tensor<BM, inDtype, LayOutType_in> DataTensor_in;
    typedef Tensor<BM, outDtype, LayOutType_out> DataTensor_out;
    typedef Tensor<BM, OpDtype, LayOutType_op> OpTensor;
    typedef typename DataTensor_in::Dtype InDataType;
    typedef typename DataTensor_out::Dtype OutDataType;
    typedef typename OpTensor::Dtype OpDataType;

    VenderFc() = default;
    ~VenderFc() {
        if (_handle != nullptr) {
            CUBLAS_CHECK(cublasDestroy(_handle));
        }
    }

    virtual SaberStatus init(const std::vector<DataTensor_in *>& inputs,
                            std::vector<DataTensor_out *>& outputs,
                            FcParam<OpTensor>& param, Context<BM>& ctx){
        // get context
        this->_ctx = ctx;
        cudaStream_t cuda_stream;
        cuda_stream = ctx.get_compute_stream();

        CUBLAS_CHECK(cublasCreate(&_handle));
        CUBLAS_CHECK(cublasSetStream(_handle, cuda_stream));
        return create(inputs, outputs, param, ctx);
    }

    virtual SaberStatus create(const std::vector<DataTensor_in *>& inputs,
                            std::vector<DataTensor_out *>& outputs,
                            FcParam<OpTensor>& param, Context<BM>& ctx){

        if (!(ctx == this->_ctx)) {
            if (_handle != NULL) {
                CUBLAS_CHECK(cublasDestroy(_handle));
            }
            this->_ctx = ctx;

            cudaStream_t cuda_stream;
            cuda_stream = ctx.get_compute_stream();
            CUBLAS_CHECK(cublasCreate(&_handle));
            CUBLAS_CHECK(cublasSetStream(_handle, cuda_stream));
        }

        Shape shape_out = inputs[0]->valid_shape();
        _M = inputs[0]->count_valid(0, param.axis);
        _K = inputs[0]->count_valid(param.axis, inputs[0]->dims());
        _N = param.num_output;
        if (_N <= 0) {
            int weight_size = param.weights->valid_size();
            _N = weight_size / _K;
        }
        //! weights dims must be in h and w
        _flag_trans_weights = param.is_transpose_weights;
        return SaberSuccess;
    }

    virtual SaberStatus dispatch(const std::vector<DataTensor_in *>& inputs,
                            std::vector<DataTensor_out *>& outputs,
                            FcParam<OpTensor>& param);


private:
    bool _flag_trans_weights{false};
    int _M;
    int _K;
    int _N;
    cublasHandle_t _handle;
    bool _is_continue_buf{true};
};

template class VenderFc<BM, AK_FLOAT, AK_FLOAT, AK_FLOAT, NCHW, NCHW, NCHW>;
} //namespace saber

} //namespace anakin

#endif //ANAKIN_SABER_FUNCS_CUDA_CUBLAS_FC_H
