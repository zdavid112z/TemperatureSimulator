# TemperatureSimulator
A tool that simulates the distribution of heat in a conductive rectangular sheet given a single heat source on it.
- The 2D rectangular sheet has a dynamic ambient temperature on the edges, which is not affected by any inner heat source
- The heat source has the shape of a single point on the sheet, with dynamic temperature and position
- The algorithm used is a modified box blur, repeatedly applying the filter and displaying how much the heat distribution has changed after each step
- Has two modes of operation: GPU Exclusive and Shared. The GPU Exclusive mode uses solely the GPU, while the Shared mode simulates a percentage of the simulation on the GPU and the rest on the CPU.
- Most simulation parameters (ambient temperature, heat source position and temperature, cpu workload etc) can be dynamically adjusted without restarting the simulation
- Can execute multiple steps in one frame to improve speed and includes profiling information to monitor the general performance

This application was developed using C++, OpenCL, Direct3D 11 and Dear ImGui for the GPGPU workshop from 3DUPB.

![](presentation.gif)
