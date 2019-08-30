# Yeah World!
A simple set of scripts to explore gesture recognition with TensorFlow and transfer learning on the Raspberry Pi and Pi Zero.

## Getting started

This is the example code used in Arm's [Raspberry Pi gesture recognition walkthrough](https://developer.arm.com/technologies/machine-learning-on-arm/developer-material/how-to-guides/teach-your-raspberry-pi-yeah-world) - full installation and usage instructions can be found there.

*Note: These example scripts are designed to be easy to read and follow, not to demonstrate state-of-the-art or best practice.*

## Dependencies

From a base Raspian install you will need to add TensorFlow:

    # TensorFlow dependencies
    sudo apt-get install libblas-dev liblapack-dev python-dev libatlas-base-dev gfortran python-setuptools python-h5py 
    
    # Pi Zero 
    sudo pip2 install http://ci.tensorflow.org/view/Nightly/job/nightly-pi-zero/lastSuccessfulBuild/artifact/output-artifacts/tensorflow-1.4.0-cp27-none-any.whl
    
    # Pi 3 (Raspbian 9) or Pi 4 Model B(Raspbian 10)
    sudo pip install --upgrade tensorflow
    # On a low memory Pi, you will probably get a "Memory Error" error message during installation. Instead, please use the command below.
    sudo pip install --no-cache-dir tensorflow
    
## Example

Check out the [full gesture recognition walkthrough](https://developer.arm.com/technologies/machine-learning-on-arm/developer-material/how-to-guides/teach-your-raspberry-pi-yeah-world). If you just need to be reminded about the syntax supported by the example scripts, read on.

Make sure the camera can see you (the rotation doesn't really matter):

    python preview.py

Clear the DISPLAY environment variable before proceeding to improve the frame rate, especially on a Pi Zero:

    unset DISPLAY

Record 15 seconds of video of yourself cheering and save it as example/yeah:
    
    python record.py example/yeah 15
    
Record 15 seconds of video of yourself sitting and save it as example/sitting:
    
    python record.py example/sitting 15
    
Record 15 seconds of video of random behaviour (walking around, covering the camera, scratching your head):
    
    python record.py example/random 15
    
Train a model to distinguish between cheering (category 0), sitting (category 1) and random (category 2):

	python train.py example/model.h5 example/yeah example/sitting example/random
	
Run a trained model and play random sounds from sounds/ when category 0 is detected:

    python run.py example/model.h5
