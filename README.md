# AmbientOcclusionRendering
Order-independent semi-transparent ambient occlusion renderers based on KVS.

## Installation

### Prerequisite
* [KVS](https://github.com/naohisas/KVS)

### Build
Clone AmbientOcclusionRendering repository from GitHub as follows:
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
The library includes the following classes used in the test programs and application programs. All of classes included in the library are defined in the namespace of `AmbientOcclusionRendering`.

* `AmbientOcclusionRendering::AmbientOcclusionBuffer`
<br>A class that facilitates buffers for screen space ambient occlusion.

* `AmbientOcclusionRendering::SSAOPolygonRenderer`
<br>Polygon renderer class with screen space ambient occlusion effect.

* `AmbientOcclusionRendering::SSAOStylizedLineRenderer`
<br>Stylized line renderer class with screen space ambient occlusion effect.

* `AmbientOcclusionRendering::SSAOStochasticPolygonRenderer`
<br>Order-independent semi-transparent polygon renderer class with screen space ambient occlusion effect.

* `AmbientOcclusionRendering::SSAOStochasticStylizedLineRenderer`
<br>Order-independent semi-transparent stylized line renderer class with screen space ambient occlusion effect. A opacity value can be specified for the streamlines.

* `AmbientOcclusionRendering::SSAOStochasticTetrahedraRenderer`
<br>Order-independent semi-transparent tetrahedra renderer class with screen space ambient occlusion effect.

* `AmbientOcclusionRendering::SSAOStochasticTubeRenderer`
<br>Order-independent semi-transparent tube renderer class with screen space ambient occlusion effect<sup>[1,2]</sup>. The opacities can be specified for each vertex of streamlines by using the transfer function.

* `AmbientOcclusionRendering::SSAOStochasticUniformGridRenderer`
<br>Order-independent semi-transparent uniform grid renderer class with screen space ambient occlusion effect<sup>[1,2]</sup>. The opacities can be specified for each vertex by using the transfer function.

## Publications
1. 藤田 泰之, 坂本 尚久, 確率的半透明流線可視化向けアンビエントオクルージョン, 第47回 可視化情報シンポジウム, 2019. [[repo](https://github.com/vizlab-kobe-paper/2019_VisSympo__YasuyukiFujita)]
2. Yasuyuki Fujita, Naohisa Sakamoto, Koji Koyamada, Ambient Occulusion for Semi-transparent Streamlines with Stochastic Rendering Technique, The 15th Asia Symposium on Visualization (ASV15), Abstract files (ASV-0205), 2019. [[repo](https://github.com/vizlab-kobe-paper/2019_ASV__YasuyukiFujita/blob/master/Submitted/abst.pdf)]
