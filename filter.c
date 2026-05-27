/**
 * Membro 1: Hugo Serra - 105054
 * Membro 2: Martim Freitas - 123306
 */
#include <inttypes.h>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Reads PGM (portable GrayMap) image
 *
 * The routine will allocate (malloc) the `img` buffer, this must be
 * deallocated explicitly once it is no longer necessary
 *
 * @note  Only bit depths of up to 255 are supported
 *
 * @param img       Output image
 * @param dims      Image dimensions
 * @param fname     File name
 * @return int
 */
int read_pgm(uint8_t **img, int dims[], char *fname) {
  FILE *fp = fopen(fname, "r");
  if (!fp) {
    perror("Unable to open file\n");
    return -1;
  }

  char buffer[1024];

  // Check magic
  fread(buffer, 1, 3, fp);
  if (buffer[0] != 'P' || buffer[1] != '5' || buffer[2] != '\n') {
    fprintf(stderr, "Unsupported file format\n");
    return -1;
  }

  // Skip comment, if any
  char comment = 0;
  do {
    fread(&comment, 1, 1, fp);
    if (comment == '#') {
      fscanf(fp, "%[^\n]s", buffer);
      // printf("%s: %s\n", fname, buffer );
    } else {
      fseek(fp, -1, SEEK_CUR);
    }
  } while (comment == '#');

  int depth;
  fscanf(fp, "%d %d %d", &dims[0], &dims[1], &depth);

  if (depth > 255) {
    fprintf(stderr, "Unsupported bit depth (%d)\n", depth);
    fclose(fp);
    return -1;
  }

  *img = malloc(dims[0] * dims[1]);
  if (*img == NULL) {
    fprintf(stderr, "Unable to allocate memory for file\n");
    fclose(fp);
    return -1;
  }

  fread(*img, 1, dims[0] * dims[1], fp);
  fclose(fp);

  return 0;
}

/**
 * @brief Write PGM (portable GrayMap) image
 *
 * @note Only bit depths of 8 bits are supported
 *
 * @param image     Image data
 * @param dims      Image dimensions
 * @param fname     File name
 */
void write_pgm(uint8_t *image, const int dims[], const char *fname) {

  FILE *f = fopen(fname, "wb");

  // Write PGM header
  fprintf(f, "P5\n%d %d\n255\n", dims[0], dims[1]);
  // Write image data
  fwrite(image, 1, dims[0] * dims[1], f);

  fclose(f);
}

/**
 * @brief Write CSV data file
 *
 * @param data      1D vector data
 * @param n         Number of elements
 * @param fname     File name
 */
void write_csv(int data[], const int n, const char *fname) {
  FILE *f = fopen(fname, "w");
  fprintf(f, "level, count\n");
  for (int i = 0; i < n; i++) {
    fprintf(f, "%d,%d\n", i, data[i]);
  }
  fclose(f);
}

/**
 * @brief Apply gaussian blur filter to image
 *
 * @param img   Image data (input / output)
 * @param nx    Image x dimension
 * @param ny    Image inner y dimension, actual buffer is expected to
 *              have ny + y cells
 */
void gauss(uint8_t *img, const int nx, const int ny) {
  /**
   * @brief Gaussian blur kernel
   *
   */
  float kernel[3][3] = {
      {0.0625, 0.125, 0.0625}, {0.125, 0.25, 0.125}, {0.0625, 0.125, 0.0625}};

  uint8_t *tmp = malloc(nx * (ny + 2));

  // Apply kernel convolution and store in tmp
  for (int j = 1; j < ny + 1; j++) {
    for (int i = 1; i < nx - 1; i++) {
      float f = 0.;
      for (int k1 = -1; k1 <= 1; k1++)
        for (int k0 = -1; k0 <= 1; k0++) {
          f += img[(j + k1) * nx + (i + k0)] * kernel[k1 + 1][k0 + 1];
        }
      tmp[j * nx + i] = f;
    }
  }

  // Copy result back
  for (int j = 1; j < ny + 1; j++) {
    for (int i = 1; i < nx - 1; i++) {
      img[j * nx + i] = tmp[j * nx + i];
    }
  }

  free(tmp);
}

/**
 * @brief Compute image histogram
 *
 * The histogram counts how many pixels of a given intensity show up on the
 * image
 *
 * @param histogram     Histogram data
 * @param img           Image data
 * @param nx    Image x dimension
 * @param ny    Image inner y dimension, actual buffer is expected to
 *              have ny + y cells
 */
void hist(int histogram[], uint8_t *restrict img, const int nx, const int ny) {

  // Set all histogram values to 0
  for (int i = 0; i < 256; i++)
    histogram[i] = 0;

  // Compute histogram
  for (int j = 1; j < ny + 1; j++)
    for (int i = 1; i < nx - 1; i++) {
      int value = img[i + nx * j];
      histogram[value]++;
    }
}

int main(int argc, char *argv[]) {
  /**
   * @brief Number of passes
   *
   */
  const int level = 32;

  MPI_Init(&argc, &argv);

  int size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (argc != 2) {
    fprintf(stderr, "Usage: %s <file.pgm>\n", argv[0]);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }

  // Image dimensions
  int dims[2];
  uint8_t *img;

  if (rank == 0) {
    int err = read_pgm(&img, dims, argv[1]);
    if (err) {
      fprintf(stderr, "Error reading file: %s\n", argv[1]);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }

    if (dims[1] % size != 0) {
      fprintf(stderr,
              "Image y-dimension (%d) does not divide evenly by number of "
              "processes (%d)\n",
              dims[1], size);
      MPI_Abort(MPI_COMM_WORLD, 1);
    }
  }

  // (*todo*) 1. Broadcast image dimensions (dims) to all processes
  MPI_Bcast(dims, 2, MPI_INT, 0, MPI_COMM_WORLD);

  int nx = dims[0];
  int ny = dims[1] / size;
  uint8_t *local_img = malloc(nx * (ny + 2));

  // (*todo*) 2. Split the image data over all processes
  MPI_Scatter(img, nx * ny, MPI_BYTE, local_img + nx, nx * ny, MPI_BYTE, 0,
              MPI_COMM_WORLD);

  // Compute histogram
  int histogram[256];
  hist(histogram, local_img, nx, ny);

  int global[256];

  // (*todo*) 4. Compute the global histogram
  MPI_Reduce(histogram, global, 256, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

  if (rank == 0) {
    write_csv(global, 256, "histogram.csv");
  }

  // Apply image filter
  for (int i = 0; i < level; i++) {
    MPI_Request lower[2], upper[2];

    // (*todo*) 5. Exchange edge values
    if (rank > 0) {
      MPI_Isend(local_img + nx, nx, MPI_BYTE, rank - 1, 0, MPI_COMM_WORLD,
                &upper[0]);
      MPI_Irecv(local_img, nx, MPI_BYTE, rank - 1, 1, MPI_COMM_WORLD,
                &upper[1]);
    }
    if (rank < size - 1) {
      MPI_Isend(local_img + nx * ny, nx, MPI_BYTE, rank + 1, 1, MPI_COMM_WORLD,
                &lower[0]);
      MPI_Irecv(local_img + nx * (ny + 1), nx, MPI_BYTE, rank + 1, 0,
                MPI_COMM_WORLD, &lower[1]);
    }

    if (rank > 0) {
      MPI_Waitall(2, upper, MPI_STATUSES_IGNORE);
    }
    if (rank < size - 1) {
      MPI_Waitall(2, lower, MPI_STATUSES_IGNORE);
    }

    // Filter
    gauss(local_img, nx, ny);
  }

  // (*todo*) 3. Gather the image from all processes on rank 0
  MPI_Gather(local_img + nx, nx * ny, MPI_BYTE, img, nx * ny, MPI_BYTE, 0,
             MPI_COMM_WORLD);

  free(local_img);

  // Save result
  if (rank == 0) {
    write_pgm(img, dims, "filter.pgm");
    free(img);
  }

  MPI_Finalize();
  return 0;
}