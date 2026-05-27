# Project 2 - MPI
**Group Members:**
- Hugo Serra - 105054
- Martim Freitas - 123306

## Hardware Environment
- **Processor:** Apple M1 Pro (10 Cores)
- **Memory/System:** macOS

## Parallelization Strategy

The parallelization of the image processing tasks was implemented using the Message Passing Interface (MPI) standard. The goal was to split the workload evenly among the available processes to reduce overall execution time.

### 1. Data Distribution
The image data is initially read by the master process (Rank 0). It broadcasts the image dimensions to all other processes using `MPI_Bcast`. The total image data is then divided horizontally and distributed across all processes using `MPI_Scatter`. Each process receives a local chunk of size `nx * ny`. An additional row is padded at the top and bottom of each local chunk to serve as "guard cells", meaning the actual buffer size allocated is `nx * (ny + 2)`.

### 2. Histogram Generation
The histogram calculation is performed locally on the scattered image data excluding the guard cells. Each process computes the count of pixel intensities in its `ny` inner rows. Once complete, `MPI_Reduce` with the `MPI_SUM` operation is used to sum all the local histograms across processes and return the global histogram back to Rank 0.

### 3. Image Filtering
The Gaussian blur filter requires values from adjacent pixels, which presents a challenge at the boundary rows of each local chunk. Before the filtering logic is applied on each iteration (32 times total), the processes must exchange their boundary data with their top and bottom neighbors:
- Each process uses `MPI_Isend` and `MPI_Irecv` to send its top inner row and receive the upper neighbor's bottom inner row (into its top guard cell).
- It similarly exchanges its bottom inner row with the lower neighbor.
- `MPI_Waitall` is used to ensure these non-blocking communications are completed before starting the convolution.

After the 32 passes of filtering, `MPI_Gather` is used to collect all the processed inner rows from every process back to Rank 0 into the final output image array.

## Performance Analysis
The performance was measured by processing the high-resolution `alex8.pgm` image on the Apple M1 Pro processor, observing the execution time with 1, 2, and 4 processes.

| Number of Processes | Total Execution Time (s) | Speedup | Efficiency |
|---------------------|--------------------------|---------|------------|
| 1                   | 40.900                   | 1.00x   | 100.0%     |
| 2                   | 20.701                   | 1.98x   | 98.8%      |
| 4                   | 10.559                   | 3.87x   | 96.8%      |

**Discussion:**
The implementation scales exceptionally well. With 2 processes, the execution time is almost perfectly halved, giving a speedup of 1.98x. With 4 processes, the execution time drops to roughly a quarter of the serial execution time, giving a speedup of 3.87x. This indicates that the workload is highly parallelizable and computationally bound. The communication overhead for guard cell exchange and data distribution/gathering is minimal compared to the heavily CPU-bound Gaussian blur computation over 32 passes.
