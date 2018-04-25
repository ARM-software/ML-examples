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
Merge multiple datasets:
    merge.py day1+2 day1 day2
The above copies all files in day1 and day2 into day1+2 whilst preserving directory structure and
introducing unique names to avoid overwriting/conflicts.
"""
from sys import argv, exit
from os import system
from glob import glob



if len(argv) < 3:
    print("""Usage: merge.py OUTPUT_DIR INPUT_DIRS...
Copy images from INPUT_DIRS to class folders in OUTPUT_DIR without overwriting duplicate names""")
    exit(1)

output = argv[1][:-1] if argv[-1] == '/' else argv[1]
directories = argv[2:]

system('mkdir -p "%s"' % output)

total = 0

for directory in directories:
    directory = directory[:-1] if directory[-1] == '/' else directory
    subdirs = glob('%s/[0-9]/' % directory)

    for subdir in sorted(subdirs):
        print('Processing %s' % subdir)
        subdir = subdir[:-1]
        label = int(subdir.split('/')[-1])
        base = subdir.split('/')[-2]
        images = glob('%s/*.png' % subdir)

        system("mkdir -p '%s/%s'" % (output, label))
        for filename in images:
            system('cp "%s" "%s/%s/%s-%s"' % (filename, output, label, base, filename.split('/')[-1]))
            total += 1

print('%d images copied to %s' % (total, output))
