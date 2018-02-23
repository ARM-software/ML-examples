ARMNN_LIB = ${HOME}/devenv/build-x86_64/release/armnn
ARMNN_INC = ${HOME}/devenv/armnn/include

all: mnist mnist_caffe

mnist: mnist.cpp mnist_loader.hpp
	g++ --g -std=c++11 -I$(ARMNN_INC) mnist.cpp -o mnist -L$(ARMNN_LIB) -larmnn -larmnnTfParser

mnist_caffe: mnist_caffe.cpp mnist_loader.hpp
	g++ --g -std=c++11 -I$(ARMNN_INC) mnist_caffe.cpp -o mnist_caffe -L$(ARMNN_LIB) -larmnn -larmnnCaffeParser

clean:
	-rm -f mnist mnist_caffe

test: mnist mnist_caffe
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(ARMNN_LIB) ./mnist 
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(ARMNN_LIB) ./mnist_caffe
