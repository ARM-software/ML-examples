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
""" Provides PiNet, a MobileNet-based feature extractor loaded from TensorFlow.
This loads many times faster than when using keras.applications, which can take
several minutes on a Pi Zero. """
from __future__ import print_function


from builtins import object
import tensorflow as tf
import numpy as np



class PiNet(object):
    """ Load a feature extractor and provide a convenience function for 1-batch
        preprocessing and inference """
    def __init__(self):
        with tf.gfile.GFile('mnet.pb', 'rb') as f:
            graph_def = tf.GraphDef()
            graph_def.ParseFromString(f.read())

        with tf.Graph().as_default() as graph:
            tf.import_graph_def(graph_def)

        self.x = graph.get_operations()[0].outputs[0]
        self.y = graph.get_operations()[-1].outputs[0]
        self.session = tf.Session(graph=graph)
        _ = self.features(np.zeros((128, 128, 3))) # warm up

    def features(self, image):
        """ Image should be a numpy array of shape width, height, channels.
            The mnet.pb feature extractor expects 128, 128, 3 in RGB format
            Returns a (4, 4, 256) numpy array representing high-level features """

        preprocessed = ((np.array(image, dtype=np.float32) / 255.) - 0.5) * 2.
        features = self.session.run(self.y, feed_dict={self.x: [image]})[0]
        return features


if __name__ == '__main__':
    darkness = np.zeros((128, 128, 3)) # NWHC
    net = PiNet()
    z = net.features(darkness)[0]
    print(z.shape)
    print(z)
