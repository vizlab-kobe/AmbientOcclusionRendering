# AmbientOcclusionRendering
Order-independent semi-transparent ambient occlusion renderers based on KVS.

## Installation

### Prerequisite
* [KVS](https://github.com/naohisas/KVS)

### Build
Clone the StochasticStreamline repository from GitHub as follows:
```bash
$ git clone https://github.com/vizlab-kobe/AmbientOcclusionRendering.git
```

#### Lib
Build AmbientOcclusionRendering library required to compile each application in App and Test.
```bash
$ cd AmbientOcclusionRendering
$ cd Lib
$ ./kvsmake.py
```

If necessary, rebuild the library as follows:
```bash
$ ./kvsmake.py rebuild
```

#### Test
Some of the test programs are in the Test directory. All of these programs can be built using the kvsmake command in each test program directory (XXX).
```bash
$ cd Test
$ cd XXX
$ kvsmake -G
$ kvsmake
$ ./run.sh
```

Note: Some of test programs require (e.g. Test/SSAOPolygonRendering) a test data, bunny.ply, for executing run.sh. These dataset can be downloaded from [KVS.data](https://github.com/naohisas/KVS.data)

#### App
The App directory contains several application programs. To build and run these application programs, refer to the ReadMe files in each directories.

## Class List
The library includes the following classes used in the test programs and application programs. All of classes included in the library are defined in the namespace of AmbientOcclusionRendering.

## Publications
1. 藤田 泰之, 坂本 尚久, 確率的半透明流線可視化向けアンビエントオクルージョン, 第47回 可視化情報シンポジウム, 2019. [[repo](https://github.com/vizlab-kobe-paper/2019_VisSympo__YasuyukiFujita)]
2. Yasuyuki Fujita, Naohisa Sakamoto, Koji Koyamada, Ambient Occulusion for Semi-transparent Streamlines with Stochastic Rendering Technique, The 15th Asia Symposium on Visualization (ASV15), (accepted), 2019. [[repo](https://github.com/vizlab-kobe-paper/2019_ASV__YasuyukiFujita/blob/master/Submitted/abst.pdf)]
