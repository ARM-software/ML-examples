ARMNN_LIB = ${HOME}/devenv/build-x86_64/release/armnn
ARMNN_INC = ${HOME}/devenv/armnn/include

all: mnist_caffe mnist_tf

mnist_caffe: mnist_caffe.cpp mnist_loader.hpp
	g++ -O3 -std=c++14 -I$(ARMNN_INC) mnist_caffe.cpp -o mnist_caffe -L$(ARMNN_LIB) -larmnn -larmnnCaffeParser

mnist_tf: mnist_tf.cpp mnist_loader.hpp
	g++ -O3 -std=c++14 -I$(ARMNN_INC) mnist_tf.cpp -o mnist_tf -L$(ARMNN_LIB) -larmnn -larmnnTfParser

clean:
	-rm -f mnist_tf mnist_caffe

test: mnist_caffe mnist_tf
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(ARMNN_LIB) ./mnist_caffe
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(ARMNN_LIB) ./mnist_tf
