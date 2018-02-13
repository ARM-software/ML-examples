ARMNN_LIB = ${HOME}/devenv/build-x86_64/release/armnn
ARMNN_INC = ${HOME}/devenv/armnn/include

mnist: mnist.cpp mnist_loader.hpp
	g++ -O0 -g -std=c++11 -I$(ARMNN_INC) mnist.cpp -o mnist -L$(ARMNN_LIB) -larmnn -larmnnTfParser

clean:
	-rm -f mnist

test: mnist
	LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(ARMNN_LIB) ./mnist 
