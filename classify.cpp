#include "tensorflow/lite/core/c/common.h"
#include "digit_cnn_model_large.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/recording_micro_interpreter.h" 
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "classify.h"
#include "image_buffer.h"
#include "fix15.h"


int best = 0;

namespace {
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    //int inference_count = 0;
    
    constexpr int kTensorArenaSize = 30*1024;
    uint8_t tensor_arena[kTensorArenaSize];
} 


extern "C" {
void cnn_setup() {
    tflite::InitializeTarget();

    model = tflite::GetModel(mnist_cnn_model_full_int_quant_tflite);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MicroPrintf("Model provided is not schema version %d not equal "
                    "to supported version %d.",
                    model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    static tflite::MicroMutableOpResolver<6> resolver;
    resolver.AddQuantize();
    resolver.AddConv2D();
    resolver.AddMaxPool2D();
    resolver.AddReshape();
    resolver.AddFullyConnected();
    resolver.AddSoftmax();

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        MicroPrintf("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);
}

void cnn_run_if_frame_ready() {
    if (!frame_ready) return;
    frame_ready = false;

    const float s = input->params.scale;
    const int zp = input->params.zero_point;

    fix15 denom_fix = float2fix15(1.0f / (255.0f * s));

    int idx = 0;

    for (int y = 0; y < 28; ++y) {
        for (int x = 0; x < 28; ++x, ++idx) {
            uint8_t px = img[y][x];

            fix15 px_fix = int2fix15(px);
            fix15 scaled_fix = multfix15(px_fix, denom_fix);

            int8_t q = fix2int15(scaled_fix) + zp;

            input->data.int8[idx] = q;
        }
    }

    if (interpreter->Invoke() != kTfLiteOk) {
        MicroPrintf("Invoke failed");
        return;
    }

    // int best = 0;
    // int8_t best_val = -128;
    // for (int i = 0; i < 10; ++i) {
    //     int8_t v = output->data.int8[i];
    //     MicroPrintf("prob of %d: %d", i, v);
    //     if (v > best_val) { best_val = v; best = i; }
    // }

    // MicroPrintf("Predicted digit: %d", best);

    float probs[10];
    for (int i = 0; i < 10; ++i) {
        int8_t q = output->data.int8[i];
        probs[i] = (q - output->params.zero_point) * output->params.scale;
        MicroPrintf("p[%d] = %f", i, probs[i]);   // prints 0-1 range
    }

    best = 0;
    for (int i = 1; i < 10; ++i)
        if (abs(output->data.int8[i]) > abs(output->data.int8[best])) best = i;
    MicroPrintf("Predicted digit: %d", best);
}
}

int return_digit(){
    return best; 
}