# Fast Inference on Arm® Ethos™-U55 NPU with Arm ML Embedded Evaluation Kit


## Arm Virtual Hardware


The Corstone-300 FVP with Ethos-U55 and Ethos-U65 and dependencies are available as part of Arm Virtual Hardware. 

###  Getting Started with Arm Virtual Hardware:

- Log into your AWS account, and navigate to Elastic Compute Cloud (EC2)
    
- Locate Images in the sidebar

- Search Public Images for Arm Virtual Hardware AMI

- Subscribe to the AMI

- Select Instance type (recommend c5.large)

- Choose a key pair or generate a new key

- Launch an EC2 instance based on the AVH Amazon Machine Image

- Connect to the EC2 Instance using ssh (Public DNS and 'ubuntu' as user) Or Virtual Network Computing (VNC):

1. Connecting to the AVH AMI using SSH:

Use the following command to connect to the instance and ensure the username is set to ubuntu. The -i specifies the
location of the AWS private key.

`$ssh -i mykey.pem ubuntu@<ec2-ip-address>`

2. Connecting to the AVH AMI using VNC:

To get access to FVP, VNC is suitable to get remote desktop access. VNC is available in the AMI

**In the AMI**

- setup VNC password. A read-only password is not required.

`$ vncpasswd`

- start a VNC

`$systemctl start vncserver@1.service`

On your local machine

If VNC is running on the EC2 instance a VNC client can be used to connect. The easiest way to connect is by forwarding
the VNC port using ssh. With this technique no additional ports need to be opened in the EC2 security group. To connect
use SSH port forwarding to avoid opening any other ports in the AWS security group

- Forward port 5901 on local machine `$ ssh -I -N –L 5901:localhost:5901 ubuntu@<ec2-ip-address>`

- Connect VNC client to port 5901

    - You will be prompted for the password
    
`$ssh -i mykey.pem -L 5901:localhost:5901 ubuntu@ <ec2-ip-address>`

- Install and run Jupyter notebook
    - you can either run the notebook on local host or through public IP 

## Minimal Dependencies on your local machine


To run the code on your local machine, ensure the following prerequisites are installed and they are available on your environment variables path.


- Corstone-300 FVP available on [armDeveloper](https://developer.arm.com/tools-and-software/simulation-models/fixed-virtual-platforms) 
- GNU Arm embedded toolchain version 10.2.1 or higher  
    -If you are using the proprietary Arm Compiler, ensure that the compiler license has been correctly configured.
- CMake version 3.15 or above 
- Make version 4.1 or above
- Python 3.6 or above 
- Python virtual environment module 
- latest pip and libsndfile
- TensorFlow version 2.0 or above
- TensorFlow Lite Micro
- xxd
- unzip
- Python Pillow



