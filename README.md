# cosim
Co-simulation of the simSCHD Static Scheduler and simSIMD Vector Processor
***

Abstract: ???

Keywords: ???

## Overview
* Co-simulation environment allows to utilize [simSCHD Static Scheduler][simschd] as a Scalar Core which controls multiple [simSIMD Vector Cores][simsimd].
* Seamless top-down approach for the development of the complete Vector Processing system

## Development Phases
1. Outline of the system structure and control procedures is developed and verified on the simSCHD platform at a high abstraction level
2. Granularity of the system structure and control procedures is increased to match the components which are available inside simSIMD vector cores 
3. simSIMD vector core or cores are then connected in place of the corresponding execution blocks for co-simulation and more detailed evaluation.

## Block Diagram of the Co-Simulation Platform
![block diagram][block_dia]

For more details and application examples please refer to [doc/cosim.pptx][full_doc]

[simschd]: https://github.com/timurkelin/simschd
[simsimd]: https://github.com/timurkelin/simsimd
[block_dia]: https://github.com/timurkelin/cosim/blob/master/doc/block_diagram.PNG
[full_doc]: https://github.com/timurkelin/cosim/tree/master/doc
