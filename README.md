# Disaggregated data-center simulator.
Currently emulates disaggregated NVMe. Sample layouts: layout-7.json: physically-attached NVMe. layout-8.json: Disaggregated pool of NVMe.
Isssues: hard-coded, NVMe bw / capacity must be greater than 1.
A rack with cores = 0 means pool of disaggregated resources. Otherwise it is assumed layout. To fix in future versions.
