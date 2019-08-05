CPPFLAGS=-DARMNN_TF_LITE_PARSER -I$(ARMNN_ROOT)/include -I$(ARMNN_ROOT)/src/backends -I$(ARMNN_ROOT)/src/armnnUtils -I$(ARMNN_ROOT)/tests -I$(BOOST_ROOT)/include
CFLAGS=-Wall -O3 -std=c++14 -fPIE

LDFLAGS=-pie -L$(ARMNN_BUILD) -L$(ARMNN_BUILD)/tests -L$(BOOST_ROOT)/lib
LDLIBS=-larmnn -larmnnTfLiteParser -linferenceTest -lboost_system -lboost_filesystem -lboost_program_options

all: mobilenetv1_quant_tflite

mobilenetv1_quant_tflite: mobilenetv1_quant_tflite.cpp model_output_labels_loader.hpp
	$(CXX) $(CPPFLAGS) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LDLIBS)

clean:
	-rm -f mobilenetv1_quant_tflite
