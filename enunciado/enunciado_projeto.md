#### Project 2 - MPI

##### Introduction

The project focuses on two common tasks in image processing, namely image filtering and histogram generation. We will be using "grayscale" images, which specify only light intensity using 1 byte per pixel, ranging from black (0) to white (255).

The filtering is performed with a standard technique that performs a convolution between a kernel (a small 3×3 matrix in our case) and the image. Each pixel in the output image is a function of the nearby pixels (including itself) using the kernel values as weights. The code does 32 passes of the so-called "Gaussian blur" kernel. The histogram calculation computes the number of pixels in the image that have a particular light intensity. This is achieved by looping through all pixels in the image and accumulating the histogram bin corresponding to the intensity of each pixel. The provided `filter.c` code implements these tasks in the `gauss()` and `hist()` functions.

Note that this is a group assignment. Students must work in groups of two elements. Groups may be formed by any two students enrolled in the Advanced Computing course.

##### Goals

The goal of this project is to parallelize the provided serial version of the image processing code using **MPI**, and to analyze the performance of the parallel program by measuring the execution time for an increasing number of processes.

##### Implementation

The parallel MPI program will operate as follows:

* Rank 0 will read the image data, and broadcast the image dimensions to all ranks. The parallelization will split the image vertically over the available processes. The local image buffer on each process includes two additional rows, one at the bottom, and one at the top, to work as guard cells.
* The histogram computation will operate on the local image data, excluding the guard cells. Once complete, a parallel reduction must be applied to return the value on rank 0, which will then save the output histogram file.
* Before filtering, all processes must exchange guard cells values with their neighbors. Note that rank 0 does not have a lower neighbor, and the last rank does not have an upper neighbor. Once the exchange is complete, filtering is applied to local image data, once again excluding guard cells. The process will be repeated `level` times.
* Once the filtering is complete, the program will gather the image data (excluding guard cells) on rank 0, which will then save the final image.

Use the provided template to write your code. The template already includes the file read/write, histogram and filter routines, as well as MPI initialization and cleanup. It also includes placeholders for you to place your code. **Do not change the structure of the code**.

Start by downloading the project file available in the e-learning (Moodle) platform and decompress it into the `labs` folder inside your Docker course image. The following files are provided:

* `filter.c` - Template for the image processing code;
* `dog8.pgm` - Small image for testing purposes;
* `alex8.pgm` - Large image for performance measuring;
* `filter.ipynb` - Simple jupyter notebook for visualizing the images and histogram.

Once complete, the `filter.c` program expects a single command line parameter with the name of the image file that is to be processed, e.g.:

```bash
$ mpiexec -np 4 ./filter dog8.ppm
```

The program will always save the results in the same files, namely `filter.pgm` (filtered image), and `histogram.csv` (histogram). If you have python (and required dependencies) installed on your system, you may use the supplied `filter.ipynb` to visualize your results.

##### 1. Add code to broadcast the image dimensions to all processes

Rank 0 must broadcast the image dimensions (2 integers) to all processes. Use `MPI_Bcast()`.

##### 2. Add code to split the image data over all processes

Use `MPI_Scatter()` for this purpose. Data size will be `nx * ny` of type `MPI_BYTE`. You should place the received data in the `local_img[]` array at position `nx`, to leave an empty row at the bottom of the local image that will serve as a guard cell.

##### 3. Add code to gather the image data from all processes on rank 0

Use `MPI_Gather()` for this purpose. The call should implement the reverse of the operation you performed in step 2. Note that the placeholder is closer to the end of the code.

_Hint_: Once you have completed this step you should test the code. If you comment the `gauss()` call, the output file should be identical to the input file.

##### 4. Compute the global histogram on rank 0

Use `MPI_Reduce()` for this purpose. The call should add the values of the `histogram[]` array available on all processes, and return the value on the `global[]` array on rank 0.

Test the parallel version with the `dog.pgm` file, and check for correctness.

##### 5. Add code to exchange edge values of the `local_img[]` array

The communication should copy the values on the lower and upper edges of `local_img[]` in a given process, to the corresponding guard cell rows of the lower and upper neighbors. Keep in mind that rank 0 does not have a lower neighbor, and the last rank does not have an upper neighbor.

You may use whatever point to point communication routines you see fit.

##### Documents to submit

The submission of the project must include both the parallel version of the serial code and a short report (maximum 2 pages) discussing the parallelization strategy followed for each of the image processing tasks, and discussing the speedup obtained from the serial version by the parallel code for the image filtering task. Remember to include a brief description of the hardware used, including the processor type and number of cores.

Both the report and the source file must clearly identify the two members of the group, including name and student number. Please submit both files to the e-learning (Moodle) platform in the section made available for this purpose ([Avaliação](https://moodle25.iscte-iul.pt/mod/page/view.php?id=80913) → Projecto 2 - MPI).

**Do not submit the test images**.