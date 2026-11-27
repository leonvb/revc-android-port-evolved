# FXAA 3.11 source and notice

`src/extras/shaders/fxaa311.h` contains the PC Quality portion of NVIDIA's
FXAA 3.11 header (Timothy Lottes), obtained from
<https://github.com/ThirteenAG/GTAIV.EFLC.FusionFix/blob/master/source/resources/FXAA3_11.h>.
The NVIDIA copyright and BSD-style three-clause license are retained in that
file and reproduced for binary distribution in `gamefiles/NVIDIA_FXAA_LICENSE.txt`.
The project-specific preamble, example wrapper and unused console code were
removed. The GLSL 130 and HLSL 3 sampling macros call a project wrapper that
clamps to the captured rectangle and computes luma from RGB, because the scene
copy does not contain luma in its alpha channel. Double-underscore identifiers
were renamed to avoid reserved GLSL identifiers. The PC Quality algorithm and
preset parameters remain from NVIDIA's source.

Desktop CMake installs `gamefiles`, Windows artifacts include it, Android
packages it under the gamefiles assets, and the Switch build/release artifacts
include the license alongside the NRO. Other binary distribution methods must
also include the notice.
