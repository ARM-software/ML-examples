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
Split a classified dataset into two - keep 90% of the files the original set for training and
create a new dataset prefixed by val_ with the other 10%.
"""
from sys import argv, exit
from os import system
from glob import glob
from random import shuffle



def main():
    if len(argv) != 2 or argv[1] == '--help':
        print("""Usage: validate_split.py DIRECTORY
Move 10% of the images in subdirectories of DIRECTORY to a new val_DIRECTORY.""")
        exit(1)

    directory = argv[1]
    subdirs = glob('%s/*/' % directory)

    moved = 0
    for subdir in sorted(subdirs):
        subdir = subdir[:-1]

        print('Splitting %s' % subdir)
        valdir = 'val_%s' % subdir
        system('mkdir -p %s' % valdir)
        images = glob('%s/*.png' % subdir)
        shuffle(images)
        k = len(images) // 10
        for i in images[:k]:
            system('mv %s val_%s' % (i, i))
        moved += k


    print('Moved %d images from %s to val_%s' % (moved, directory, directory))


if __name__ == '__main__':
    main()
