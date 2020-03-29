
```
   ___                              _ __       __
  / _ \_______ _______  __ _  ___  (_) /__ ___/ /
 / ___/ __/ -_) __/ _ \/  ' \/ _ \/ / / -_) _  / 
/_/  /_/  \__/\__/\___/_/_/_/ .__/_/_/\__/\_,_/  
                           /_/                   
           ______           __       
          / __/ /  ___ ____/ /__ ____
         _\ \/ _ \/ _ `/ _  / -_) __/
        /___/_//_/\_,_/\_,_/\__/_/   
          ___  _          ___         
         / _ \(_)__  ___ / (_)__  ___ 
        / ___/ / _ \/ -_) / / _ \/ -_)
       /_/  /_/ .__/\__/_/_/_//_/\__/ 
             /_/                      

```   
- [PrecompiledShaderPipeline](#precompiledshaderpipeline)
  - [Getting Started](#getting-started)
    - [Prerequisites](#prerequisites)
    - [Installing](#installing)
  - [Running the tests](#running-the-tests)
  - [Contributing](#contributing)
  - [Versioning](#versioning)
  - [Authors](#authors)
  - [License](#license)
  - [Acknowledgments](#acknowledgments)

# PrecompiledShaderPipeline

![PrecompiledShaderPipeline macOS](https://github.com/VladSerhiienko/PrecompiledShaderPipeline/workflows/PrecompiledShaderPipeline%20macOS/badge.svg?branch=master&event=push)

Offline shader compilation for different platforms with SPIR-V reflection.
Attached SPIR-V reflection contains information about stage inputs and outputs for matching shader stages,
constants, samplers, textures, buffers, and their states. For active buffer bindings and push constants,
the compiled library provides information about active ranges and paddings for structure members
to make sure developers transfer only accessed data.

```
// Reflected constants, offsets, sizes, active ranges, paddings, structures.
// Float4x4[72|?] - Array size depends on constants.
// Float4x4[72|!] - Array size is final.
Variant: asset="shaders/SceneSkinnedTest.vert", definitions=""
+ constants (2):
├ - kBoneCount
│   └── Int (size=4)
└ - kConstScalar
    └── Double (size=8)
+ stage inputs (6):
├ - inPosition (set=?, binding=?, location=0, is_active=yes)
│   └── Float3 (size=12)
├ - inBoneWeights (set=?, binding=?, location=4, is_active=yes)
│   └── Float4 (size=16)
├ - inBoneIndices (set=?, binding=?, location=5, is_active=yes)
│   └── Float4 (size=16)
├ - inTexcoords (set=?, binding=?, location=3, is_active=yes)
│   └── Float2 (size=8)
├ - inNormal (set=?, binding=?, location=1, is_active=yes)
│   └── Float3 (size=12)
└ - inTangent (set=?, binding=?, location=2, is_active=yes)
    └── Float4 (size=16)
+ stage outputs (6):
├ - outWorldPosition (set=?, binding=?, location=0, is_active=yes)
│   └── Float3 (size=12)
├ - outViewDirection (set=?, binding=?, location=4, is_active=yes)
│   └── Float3 (size=12)
├ - outTexcoords (set=?, binding=?, location=5, is_active=yes)
│   └── Float2 (size=8)
├ - outWorldNormal (set=?, binding=?, location=1, is_active=yes)
│   └── Float3 (size=12)
├ - outWorldTangent (set=?, binding=?, location=2, is_active=yes)
│   └── Float3 (size=12)
└ - outWorldBitangent (set=?, binding=?, location=3, is_active=yes)
    └── Float3 (size=12)
+ uniform buffers (2):
├ - <unnamed> (set=0, binding=0, location=?, active_ranges=[(0:192)])
│   └── CameraUBO (size=256)
│       ├── ViewMatrix : Float4x4 (offset=0, size=64)
│       ├── ProjMatrix : Float4x4 (offset=64, size=64)
│       ├── InvViewMatrix : Float4x4 (offset=128, size=64)
│       └── InvProjMatrix : Float4x4 (offset=192, size=64)
└ - <unnamed> (set=1, binding=0, location=?, active_ranges=[(0:9344),(9360:16),(9392:32)])
    └── ObjectUBO (size=9460)
        ├── WorldMatrix : Float4x4 (offset=0, size=64)
        ├── BoneOffsetMatrices : Float4x4[72|?] (offset=64, size=4608)
        ├── NormalMatrix : Float4x4 (offset=4672, size=64)
        ├── BoneNormalMatrices : Float4x4[72|?] (offset=4736, size=4608)
        ├── OtherPositionOffset : Float3 (offset=9344, effective_size=12, total_size=16, padding=4)
        ├── PositionOffset : Float4 (offset=9360, size=16)
        ├── OtherPositionScale : Float (offset=9376, effective_size=4, total_size=16, padding=12)
        ├── PositionScale : Float4 (offset=9392, size=16)
        ├── TexcoordOffsetScale : Float4 (offset=9408, size=16)
        ├── OtherSampleStruct : SampleStruct (offset=9424, size=32)
        │   ├── x : Float (offset=0, size=4)
        │   ├── y : Float (offset=4, size=4)
        │   ├── z : Float (offset=8, size=4)
        │   ├── w : Float (offset=12, size=4)
        │   └── vxyzw : Float4 (offset=16, size=16)
        └── OtherTexcoordOffsetScale : Float (offset=9456, size=4)

// Inactive inputs.
Variant: asset="shaders/Skybox.frag", definitions=""
+ stage inputs (4):
├ - v_dir (set=?, binding=?, location=0, is_active=yes)
│   └── Float3 (size=12)
├ - v_params0 (set=?, binding=?, location=1, is_active=no)
│   └── Float4 (size=16)
├ - v_params1 (set=?, binding=?, location=2, is_active=no)
│   └── Float4 (size=16)
└ - v_texcoords (set=?, binding=?, location=3, is_active=no)
    └── Float2 (size=8)
+ stage outputs (1):
└ - o_color (set=?, binding=?, location=0, is_active=yes)
    └── Float4 (size=16)
+ sampled images (2):
├ - u_samplerCube0 (set=0, binding=1, location=?, is_active=yes)
│   └── SampledImage
└ - u_samplerCube1 (set=0, binding=2, location=?, is_active=no)
    └── SampledImage
```

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

Currently, it's only tested on `macOS Mojave`, `Xcode 11.3.1`, `Build version 11C504`, `macosx10.15`, `cmake version 3.16.2`, `Python 2.7.10`.
Also, I've added GitHub Actions to ensure the automated builds and tests are running on master branch.
Building the tool from source requires `Python 2.7.x` installed on your local machine, `cmake` (https://cmake.org/download/), and `git`.

```
sudo easy_install pip
pip install future
```

### Installing

For installing the tool on `macOS`, run scripts.

```
sh cso-install-required-python.sh
sh cso-macos-xcode-build-install.sh
sh cso-update-binaries-macos.sh
```
The compiled binaries will be copied to `bin/macOS` folder.

## Running the tests

For running tests, `cd` to `bin/macOS` and run `PrecompiledShaderPipelineTests`.
Please note, that tests have hardcoded relative paths and running them from a different working directory is currently not supported.

```
sh cso-run-all-tests.sh
```

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests.


## Versioning

Use [SemVer](http://semver.org/) for versioning. 

## Authors

**Vladyslav Serhiienko** - https://github.com/VladSerhiienko

## License

This project is licensed under the MIT License, see [LICENSE.md](LICENSE.md) for details.

## Acknowledgments

* https://github.com/google/shaderc - Compilation to SPIR-V.
* https://github.com/google/flatbuffers - Reading/writing.
* https://github.com/google/googletest - Testing.
* https://github.com/KhronosGroup/SPIRV-Cross - Cross-language conversions, reflection.
* https://github.com/gabime/spdlog - Logging.
* https://github.com/adishavit/argh - Parsing inputs.

