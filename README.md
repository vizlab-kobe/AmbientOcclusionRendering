# AmbientOcclusionRendering
AmbientOcclusionRendering is a library includes particle-based renderering techniques with ambient occlusion effects.

## Requirements
* [KVS](https://github.com/naohisas/KVS)

## Compiling
Clone the repository.
```bash
$ git clone https://github.com/vizlab-kobe/AmbientOcclusionRendering.git
```

Build AmbientOcclusionRendering library required to compile each application in App.
```bash
$ cd Lib
$ ./kvsmake.py
```

Build applications with kvsmake in each test or application directory. e.g.) Test/SSAOPolygonRendering
```bash
$ cd Test/SSAOPolygonRendering
$ kvsmake -G
$ kvsmake
$ ./run.sh
```
bunny.ply is required in this script. That can be downloaded from [KVS.data](https://github.com/naohisas/KVS.data)

## Publications

1. 藤田 泰之, 坂本 尚久, 確率的半透明流線可視化向けアンビエントオクルージョン, 第47回 可視化情報シンポジウム, 2019. [[repo](https://github.com/vizlab-kobe-paper/2019_VisSympo__YasuyukiFujita)]
2. Yasuyuki Fujita, Naohisa Sakamoto, Koji Koyamada, Ambient Occulusion for Semi-transparent Streamlines with Stochastic Rendering Technique, The 15th Asia Symposium on Visualization (ASV15), (accepted), 2019. [[repo](https://github.com/vizlab-kobe-paper/2019_ASV__YasuyukiFujita/blob/master/Submitted/abst.pdf)]
