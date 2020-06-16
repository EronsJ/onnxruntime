#include "helper.h"
#include "Shaper.h"
#include "core/providers/nnapi/nnapi_builtin/nnapi_lib/NeuralNetworksWrapper.h"

using std::string;
using std::vector;

Shaper::len_t Shaper::total(const Shape& shape) {
  return Product(shape);
}

void Shaper::Conv(const std::string& input_name,
                  const std::string& weight_name,
                  const vector<int32_t>& onnx_pads,
                  const vector<int32_t>& onnx_strides,
                  const vector<int32_t>& onnx_dilations,
                  bool nchw,
                  const std::string& output_name) {
  Shape weightDimen =
      shape_map_.at(weight_name);  // num_output, height, width, num_input

  int32_t padding_left = onnx_pads[1];
  int32_t padding_right = onnx_pads[3];
  int32_t padding_top = onnx_pads[0];
  int32_t padding_bottom = onnx_pads[2];
  int32_t stride_x = onnx_strides[1];
  int32_t stride_y = onnx_strides[0];
  int32_t dilation_x = onnx_dilations[1];
  int32_t dilation_y = onnx_dilations[0];

  // NHWC
  Shape inputDimen = shape_map_.at(input_name);
  Shape outputDimen;
  if (nchw) {
    outputDimen =
        {
            inputDimen[0],
            weightDimen[0],
            inputDimen[2] == 0
                ? 0
                : (inputDimen[2] - ((weightDimen[1] - 1) * dilation_y + 1) +
                   padding_top + padding_bottom) /
                          stride_y +
                      1,
            inputDimen[3] == 0
                ? 0
                : (inputDimen[3] - ((weightDimen[2] - 1) * dilation_x + 1) +
                   padding_left + padding_right) /
                          stride_x +
                      1,
        };
  } else {  // nhwc
    outputDimen =
        {
            inputDimen[0],
            inputDimen[1] == 0
                ? 0
                : (inputDimen[1] - ((weightDimen[1] - 1) * dilation_y + 1) +
                   padding_top + padding_bottom) /
                          stride_y +
                      1,
            inputDimen[2] == 0
                ? 0
                : (inputDimen[2] - ((weightDimen[2] - 1) * dilation_x + 1) +
                   padding_left + padding_right) /
                          stride_x +
                      1,
            weightDimen[0],
        };
  }

  shape_map_[output_name] = outputDimen;

  if (!shaper_finalized_) {
    shape_ops_.push_back(
        [input_name, weight_name,
         onnx_pads, onnx_strides, onnx_dilations,
         nchw,
         output_name](Shaper& shaper) {
          shaper.Conv(input_name, weight_name,
                      onnx_pads, onnx_strides, onnx_dilations,
                      nchw,
                      output_name);
        });
  }

  // LOGV("Conv %s nchw %d", input_name.c_str(), nchw);
  // LOGV("input %d %d %d %d", inputDimen[0], inputDimen[1], inputDimen[2], inputDimen[3]);
  // LOGV("output %d %d %d %d", outputDimen[0], outputDimen[1], outputDimen[2], outputDimen[3]);
  // LOGV("weight %d %d %d %d", weightDimen[0], weightDimen[1], weightDimen[2], weightDimen[3]);
}

void Shaper::DepthwiseConv(const std::string& input_name,
                           const std::string& weight_name,
                           const std::vector<int32_t>& onnx_pads,
                           const std::vector<int32_t>& onnx_strides,
                           const std::vector<int32_t>& onnx_dilations,
                           bool nchw,
                           const std::string& output_name) {
  Shape weightDimen =
      shape_map_.at(weight_name);  // 1, height, width, num_output

  int32_t padding_left = onnx_pads[1];
  int32_t padding_right = onnx_pads[3];
  int32_t padding_top = onnx_pads[0];
  int32_t padding_bottom = onnx_pads[2];
  int32_t stride_x = onnx_strides[1];
  int32_t stride_y = onnx_strides[0];
  int32_t dilation_x = onnx_dilations[1];
  int32_t dilation_y = onnx_dilations[0];

  // NHWC
  Shape inputDimen = shape_map_.at(input_name);
  Shape outputDimen;
  if (nchw) {
    outputDimen =
        {
            inputDimen[0],
            weightDimen[3],
            inputDimen[2] == 0
                ? 0
                : (inputDimen[2] - ((weightDimen[1] - 1) * dilation_y + 1) +
                   padding_top + padding_bottom) /
                          stride_y +
                      1,
            inputDimen[3] == 0
                ? 0
                : (inputDimen[3] - ((weightDimen[2] - 1) * dilation_x + 1) +
                   padding_left + padding_right) /
                          stride_x +
                      1,
        };
  } else {  // nhwc
    outputDimen =
        {
            inputDimen[0],
            inputDimen[1] == 0
                ? 0
                : (inputDimen[1] - ((weightDimen[1] - 1) * dilation_y + 1) +
                   padding_top + padding_bottom) /
                          stride_y +
                      1,
            inputDimen[2] == 0
                ? 0
                : (inputDimen[2] - ((weightDimen[2] - 1) * dilation_x + 1) +
                   padding_left + padding_right) /
                          stride_x +
                      1,
            weightDimen[3],
        };
  }
  shape_map_[output_name] = outputDimen;

  if (!shaper_finalized_) {
    shape_ops_.push_back(
        [input_name, weight_name,
         onnx_pads, onnx_strides, onnx_dilations,
         nchw,
         output_name](Shaper& shaper) {
          shaper.DepthwiseConv(input_name, weight_name,
                               onnx_pads, onnx_strides, onnx_dilations,
                               nchw,
                               output_name);
        });
  }
  // LOGV("DepthwiseConv %s nchw %d", input_name.c_str(), nchw);
  // LOGV("input %d %d %d %d", inputDimen[0], inputDimen[1], inputDimen[2], inputDimen[3]);
  // LOGV("output %d %d %d %d", outputDimen[0], outputDimen[1], outputDimen[2], outputDimen[3]);
  // LOGV("weight %d %d %d %d", weightDimen[0], weightDimen[1], weightDimen[2], weightDimen[3]);
}

void Shaper::Pool(const std::string& input_name,
                  const std::vector<int32_t>& onnx_pads,
                  const std::vector<int32_t>& onnx_strides,
                  const std::vector<int32_t>& kernel_shape,
                  bool nchw,
                  const std::string& output_name) {
  auto inputDimen = shape_map_.at(input_name);

  int32_t padding_left = onnx_pads[1];
  int32_t padding_right = onnx_pads[3];
  int32_t padding_top = onnx_pads[0];
  int32_t padding_bottom = onnx_pads[2];
  int32_t stride_x = onnx_strides[1];
  int32_t stride_y = onnx_strides[0];
  int32_t width = kernel_shape[1];
  int32_t height = kernel_shape[0];

  Shape outputDimen;
  if (nchw) {
    outputDimen = {
        inputDimen[0],
        inputDimen[1],
        inputDimen[2] == 0
            ? 0
            : (inputDimen[2] - height + padding_top + padding_bottom) / stride_y + 1,
        inputDimen[3] == 0
            ? 0
            : (inputDimen[3] - width + padding_left + padding_right) / stride_x + 1,
    };
  } else {
    outputDimen = {
        inputDimen[0],
        inputDimen[1] == 0
            ? 0
            : (inputDimen[1] - height + padding_top + padding_bottom) / stride_y + 1,
        inputDimen[2] == 0
            ? 0
            : (inputDimen[2] - width + padding_left + padding_right) / stride_x + 1,
        inputDimen[3]};
  }

  shape_map_[output_name] = outputDimen;

  if (!shaper_finalized_) {
    shape_ops_.push_back(
        [input_name,
         onnx_pads, onnx_strides, kernel_shape,
         nchw,
         output_name](Shaper& shaper) {
          shaper.Pool(input_name,
                      onnx_pads, onnx_strides, kernel_shape,
                      nchw,
                      output_name);
        });
  }
  // LOGV("Pool %s nchw %d", input_name.c_str(), nchw);
  // LOGV("input %d %d %d %d", inputDimen[0], inputDimen[1], inputDimen[2], inputDimen[3]);
  // LOGV("output %d %d %d %d", outputDimen[0], outputDimen[1], outputDimen[2], outputDimen[3]);
}

void Shaper::Reshape(const std::string& input_name,
                     const std::vector<int32_t>& shape,
                     const std::string& output_name) {
  auto input_dimen = shape_map_.at(input_name);
  int64_t input_size = Product(input_dimen);
  std::vector<uint32_t> output_dimen(shape.size());

  int64_t capacity = 1;
  int unk_dim_idx = -1;
  for (size_t i = 0; i < shape.size(); i++) {
    int32_t dim_i = shape[i];
    if (dim_i == -1) {
      if (unk_dim_idx != -1)
        throw std::invalid_argument(
            "Only one input dimension of Attr(shape) can be unknown!");
      unk_dim_idx = i;
    } else if (dim_i == 0) {
      throw std::invalid_argument(
          "NNAPI does not support 0 reshape dimension");
    } else {
      capacity *= dim_i;
      output_dimen[i] = static_cast<uint32_t>(dim_i);
    }
  }

  if (unk_dim_idx != -1) {
    if (input_size == 0)
      output_dimen[unk_dim_idx] = 0;
    else
      output_dimen[unk_dim_idx] = input_size / capacity;

    capacity *= output_dimen[unk_dim_idx];
  }

  if (capacity != input_size)
    throw std::invalid_argument("Invalid shape is given!");

  shape_map_[output_name] = output_dimen;

  if (!shaper_finalized_) {
    shape_ops_.push_back(
        [input_name, shape, output_name](Shaper& shaper) {
          shaper.Reshape(input_name, shape, output_name);
        });
  }
}

void Shaper::Transpose(const std::string& input_name,
                       const std::vector<int32_t>& perm,
                       const std::string& output_name) {
  auto input_dimen = shape_map_.at(input_name);
  if (!perm.empty() && perm.size() != input_dimen.size())
    throw std::invalid_argument("Invalid perm is given!");
  size_t size = input_dimen.size();
  Shape output_Dimen(size);
  for (size_t i = 0; i < size; i++)
    output_Dimen[i] = input_dimen[perm.empty() ? size - i - 1 : perm[i]];

  shape_map_[output_name] = output_Dimen;

  if (!shaper_finalized_) {
    shape_ops_.push_back(
        [input_name, perm, output_name](Shaper& shaper) {
          shaper.Transpose(input_name, perm, output_name);
        });
  }
}

void Shaper::Eltwise(const std::string& input1_name,
                     const std::string& input2_name,
                     const std::string& output_name) {
  auto& shape1 = shape_map_.at(input1_name);
  auto& shape2 = shape_map_.at(input2_name);

  // broadcasting support
  bool shape1IsBigger = shape1.size() >= shape2.size();
  auto max_shape = shape1IsBigger ? shape1 : shape2;
  auto min_shape = shape1IsBigger ? shape2 : shape1;
  for (int i = (int)max_shape.size() - 1, j = (int)min_shape.size() - 1;
       i >= 0 && j >= 0;
       i--, j--) {
    if (max_shape[i] < min_shape[j])
      max_shape[i] = min_shape[j];
  }

  shape_map_[output_name] = max_shape;

  if (!shaper_finalized_) {
    shape_ops_.push_back(
        [input1_name, input2_name, output_name](Shaper& shaper) {
          LOGV("lambda eltwise input1 %s, input2 %s, output %s",
               input1_name.c_str(), input2_name.c_str(), output_name.c_str());

          shaper.Eltwise(input1_name, input2_name, output_name);
        });
  }
  // LOGV("Eltwise input1 %s", input1_name.c_str());
  // LOGV("Eltwise input2 %s", input2_name.c_str());
  // LOGV("input1 %d %d %d %d", shape1[0], shape1[1], shape1[2], shape1[3]);
  // LOGV("input2 %d %d %d %d", shape2[0], shape2[1], shape2[2], shape2[3]);
  // LOGV("output %d %d %d %d", max_shape[0], max_shape[1], max_shape[2], max_shape[3]);
}

void Shaper::Identity(const std::string& input_name,
                      const std::string& output_name) {
  shape_map_[output_name] = shape_map_.at(input_name);

  if (!shaper_finalized_) {
    shape_ops_.push_back(
        [input_name, output_name](Shaper& shaper) {
          shaper.Identity(input_name, output_name);
        });
  }
}

void Shaper::FC(const std::string& input1_name, const std::string& input2_name,
                const std::string& output_name) {
  // Currently we only support A*B'+C
  auto input1_dimen = shape_map_.at(input1_name);
  Shape input2_dimen = shape_map_.at(input2_name);  // num_units, input_size
  Shape outputDimen{input1_dimen[0], input2_dimen[0]};
  shape_map_[output_name] = outputDimen;

  if (!shaper_finalized_) {
    shape_ops_.push_back(
        [input1_name, input2_name, output_name](Shaper& shaper) {
          shaper.FC(input1_name, input2_name, output_name);
        });
  }
}

void Shaper::AddShape(const std::string& name, const Shape& shape) {
  shape_map_[name] = shape;
}

void Shaper::UpdateShape(const std::string& name, const Shape& new_shape) {
  if (!shaper_finalized_) {
    throw std::invalid_argument(
        "Cannot UpdateShape while shaper is not finalized");
  }

  const auto& old_shape = shape_map_.at(name);
  if (old_shape != new_shape && Product(shape_map_.at(name)) != 0)
    throw std::invalid_argument(
        "The shape should be same size or old shape has size 0");

  shape_map_[name] = new_shape;
}

void Shaper::UpdateDynamicDimensions() {
  if (!shaper_finalized_) {
    throw std::invalid_argument(
        "Cannot UpdateDynamicDimensions while shaper is not finalized");
  }

  for (auto& shape_op : shape_ops_)
    shape_op(*this);
}

size_t Shaper::GetSize(const std::string& name) const {
  return static_cast<size_t>(Product(shape_map_.at(name)));
}

void Shaper::Clear() {
  shaper_finalized_ = false;
  shape_map_.clear();
  shape_ops_.clear();
}