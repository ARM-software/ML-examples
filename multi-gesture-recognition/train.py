# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
"""
Train a model from scratch using a training and validation dataset:
    python train.py day1 val_day1
The best model found during training (as measured by its performance on the validation dataset0) is
saved as day1/model.h5. Not really suitable for a Raspberry Pi. Copy the dataset to a more powerful
machine (e.g. laptop or desktop) and run it there.
"""
from sys import argv, exit
from glob import glob

from keras.preprocessing.image import ImageDataGenerator
from keras.models import Sequential
from keras.layers import Conv2D, MaxPooling2D
from keras.layers import Activation, Dropout, Flatten, Dense
from keras.callbacks import EarlyStopping, ModelCheckpoint, ReduceLROnPlateau



def main():
    if len(argv) != 3 or argv[1] == '--help':
        print("""Usage: train.py TRAIN_DIR VAL_DIR...
Save TRAIN_DIR/model.h5 after training a conv net to distinguish between images in its subdirs.""")
        exit(1)

    train_data_dir = argv[1]
    val_data_dir = argv[2]
    nb_train_samples = len(glob('%s/*/*.png' % train_data_dir))
    nb_classes = len(glob('%s/*/' % train_data_dir))
    batch_size = 100

    model = Sequential()
    model.add(Conv2D(32, (3, 3), input_shape=(128, 128, 3)))
    model.add(Activation('elu'))
    model.add(MaxPooling2D(pool_size=(2, 2)))

    model.add(Conv2D(32, (3, 3)))
    model.add(Activation('elu'))
    model.add(MaxPooling2D(pool_size=(2, 2)))

    model.add(Conv2D(64, (3, 3)))
    model.add(Activation('elu'))
    model.add(MaxPooling2D(pool_size=(2, 2)))

    model.add(Flatten())
    model.add(Dense(64))
    model.add(Activation('elu'))
    model.add(Dropout(0.5))
    model.add(Dense(nb_classes))
    model.add(Activation('softmax'))

    model.compile(loss='categorical_crossentropy',
                  optimizer='adam',
                  metrics=['accuracy'])

    train_datagen = ImageDataGenerator(
        rescale=1./255,
        shear_range=0.2,
        zoom_range=0.2,
        horizontal_flip=False)

    val_datagen = ImageDataGenerator(rescale=1. / 255)

    train_generator = train_datagen.flow_from_directory(
        train_data_dir,
        target_size=(128, 128),
        batch_size=batch_size,
        class_mode='categorical')

    val_generator = val_datagen.flow_from_directory(
        val_data_dir,
        target_size=(128, 128),
        batch_size=batch_size,
        class_mode='categorical')

    model.fit_generator(
        train_generator,
        steps_per_epoch=nb_train_samples // batch_size,
        epochs=100,
        validation_data=val_generator,
        validation_steps=10,
        callbacks=[ModelCheckpoint('%s/model.h5' % train_data_dir, save_best_only=True),
                   EarlyStopping(patience=10),
                   ReduceLROnPlateau(factor=0.2, patience=5, verbose=1)])

if __name__ == '__main__':
    main()
