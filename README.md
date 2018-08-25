# S4Sim - Smart Solid State Storage Simulation  


# Quick Start

### Install dependent packages

```

  sudo apt  install build-essential crossbuild-essential-armel crossbuild-essential-armhf openssh-server qemu scons swig python-dev python-pydot graphviz graphviz-dev git libssh2-1-dev libvdeplug-dev libsnappy-dev librdmacm-dev libgoogle-perftools-dev m4 pkg-config libjpeg-turbo8-dev libglib2.0-dev libpng16-dev bison 


```

### Download and compile gem5 simulator
 
```

  cd SSD_hw_platforms/gem5/
  wget https://github.com/gem5/gem5/archive/stable_2015_09_03.tar.gz
  tar -xzf stable_2015_09_03.tar.gz
  cd gem5-stable_2015_09_03
  scons build/ARM/gem5.opt EXTRAS=../extra_sim_objects/ -j8


```


### Download, patch and compile Qemu for host system simulation

```

  cd Host_hw_platforms/qemu/
  wget https://download.qemu.org/qemu-2.8.1.1.tar.xz
  tar -xf qemu-2.8.1.1.tar.xz 
  cd qemu-2.8.1.1
  patch -p1 <../s4sim-qemu-host.patch 
  ./configure  --target-list=x86_64-softmmu --enable-vnc-jpeg --enable-vnc-png --enable-kvm 
  make -j8


```


### Compile the S4Sim Firmware 

```

  make 


```


## Running the Simulation with the provided images

### Download Images 
1. 256MB Exfat SSD Image 
2. Ubuntu Host Image

```
     
  cd disk_images
  ./download-exfat-img.sh
  ./download-host-img.sh
  

```  

### Start the Host System

```

  ./starthost.sh 


```

Host system ssh login username:user , password:user

### Start the SSD Simulator

```

  make run
  
  
```  

### Connect the SSD and Host Interfaces
```

  ./connect_nvme2host.sh
  

```  



