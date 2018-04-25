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
Test the model trained on one dataset on the images in another dataset and report its accuracy and
the images that it mispredicted:
    python test.py day1 day2
The above assumes day1/model.h5 has been created with train.py.
"""
from sys import argv, exit
from glob import glob
from collections import defaultdict

import numpy as np
from keras.models import load_model
from PIL import Image



def main():
    if len(argv) != 3 or argv[1] == '--help':
        print("""Usage: test.py TRAIN_DIR TEST_DIR...
Load TRAIN_DIR/model.h5 and report on its accuracy and mispredicted items from TEST_DIR.""")
        exit(1)

    train_data_dir = argv[1]
    test_data_dir = argv[2]
    image_files = glob('%s/*/*.png' % test_data_dir)

    model = load_model('%s/model.h5' % train_data_dir)

    correct = 0
    correct_by_class = defaultdict(int)
    total_by_class = defaultdict(int)
    for filename in sorted(image_files):
        try:
            image = Image.open(filename)
            x = np.array(image.getdata()).reshape((128, 128, 3)) / 255.0
            l = int(filename.split('/')[-2])
            p = model.predict([np.array([x])])[0]
            y = np.argmax(p)
            if y == l:
                correct += 1
                correct_by_class[l] += 1
            else:
                print('%s: actual %d, predicted %d (%s)' % (filename, l, y,
                                                            ', '.join('%.1f' % d for d in p)))
            total_by_class[l] += 1
        except:
            print('Error processing %s' % filename)

    for c in total_by_class.keys():
        print('Class %d: %5d/%5d: %.0f %% accurate' % (c, correct_by_class[c], total_by_class[c],
                                                       100.0 * correct_by_class[c] / total_by_class[c]))

    print('Overall  %5d/%5d: %.0f %% accurate' % (correct, len(image_files),
                                                  100.0* correct / len(image_files)))


if __name__ == '__main__':
    main()
