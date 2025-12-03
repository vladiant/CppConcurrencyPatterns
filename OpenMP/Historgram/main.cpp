#include <omp.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <random>

void histogram_openmp(size_t hist[256], uint8_t* image, size_t n) {
#pragma omp parallel shared(hist, image, n) default(none)
  {
    size_t hist_priv[256]{};
#pragma omp for nowait
    for (size_t i = 0; i < n; i++) {
      hist_priv[image[i]]++;
    }
#pragma omp critical
    for (size_t i = 0; i < 256; i++) {
      hist[i] += hist_priv[i];
    }
  }
}

void histogram_sequential(size_t hist[256], uint8_t* image, size_t n) {
  for (size_t i = 0; i < n; i++) {
    hist[image[i]]++;
  }
}

// Generate a random image
uint8_t* generate_image(size_t width, size_t height) {
  size_t size = width * height;
  uint8_t* image = new uint8_t[size];

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> dist(0, 255);

  for (size_t i = 0; i < size; i++) {
    image[i] = static_cast<uint8_t>(dist(gen));
  }

  return image;
}

// Compare two histograms
bool compare_histograms(size_t hist1[256], size_t hist2[256]) {
  for (int i = 0; i < 256; i++) {
    if (hist1[i] != hist2[i]) {
      return false;
    }
  }
  return true;
}

// Print histogram statistics
void print_histogram_stats(size_t hist[256], size_t total_pixels) {
  printf("Histogram statistics:\n");

  // Find min and max frequencies
  size_t min_freq = hist[0];
  size_t max_freq = hist[0];
  int min_val = 0;
  int max_val = 0;

  for (int i = 0; i < 256; i++) {
    if (hist[i] < min_freq) {
      min_freq = hist[i];
      min_val = i;
    }
    if (hist[i] > max_freq) {
      max_freq = hist[i];
      max_val = i;
    }
  }

  printf("  Total pixels: %zu\n", total_pixels);
  printf("  Most frequent value: %d (count: %zu, %.2f%%)\n", max_val, max_freq,
         100.0 * max_freq / total_pixels);
  printf("  Least frequent value: %d (count: %zu, %.2f%%)\n", min_val, min_freq,
         100.0 * min_freq / total_pixels);

  // Show first few histogram values
  printf("  First 10 values: ");
  for (int i = 0; i < 10; i++) {
    printf("%zu ", hist[i]);
  }
  printf("\n");
}

int main() {
  printf("Start\n");
  printf("Number of threads: %d\n\n", omp_get_max_threads());

  // Generate test image
  size_t width = 4096;
  size_t height = 4096;
  size_t size = width * height;

  printf("Generating %zux%zu image (%zu pixels)...\n", width, height, size);
  auto start_gen = std::chrono::high_resolution_clock::now();
  uint8_t* image = generate_image(width, height);
  auto end_gen = std::chrono::high_resolution_clock::now();
  auto duration_gen = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_gen - start_gen);
  printf("Generated in %ld ms\n\n", duration_gen.count());

  // Compute histogram using OpenMP
  size_t hist_parallel[256]{};
  printf("Computing histogram (parallel)...\n");
  auto start_parallel = std::chrono::high_resolution_clock::now();
  histogram_openmp(hist_parallel, image, size);
  auto end_parallel = std::chrono::high_resolution_clock::now();
  auto duration_parallel =
      std::chrono::duration_cast<std::chrono::microseconds>(end_parallel -
                                                            start_parallel);
  printf("Parallel time: %ld μs (%.3f ms)\n", duration_parallel.count(),
         duration_parallel.count() / 1000.0);
  print_histogram_stats(hist_parallel, size);
  printf("\n");

  // Compute histogram sequentially for verification
  size_t hist_sequential[256]{};
  printf("Computing histogram (sequential)...\n");
  auto start_sequential = std::chrono::high_resolution_clock::now();
  histogram_sequential(hist_sequential, image, size);
  auto end_sequential = std::chrono::high_resolution_clock::now();
  auto duration_sequential =
      std::chrono::duration_cast<std::chrono::microseconds>(end_sequential -
                                                            start_sequential);
  printf("Sequential time: %ld μs (%.3f ms)\n", duration_sequential.count(),
         duration_sequential.count() / 1000.0);
  print_histogram_stats(hist_sequential, size);
  printf("\n");

  // Verify results match
  if (compare_histograms(hist_parallel, hist_sequential)) {
    printf("✓ Results match!\n");
  } else {
    printf("✗ Results differ!\n");
    // Show differences
    for (int i = 0; i < 256; i++) {
      if (hist_parallel[i] != hist_sequential[i]) {
        printf("  Value %d: parallel=%zu, sequential=%zu\n", i,
               hist_parallel[i], hist_sequential[i]);
      }
    }
  }

  // Calculate speedup
  double speedup = static_cast<double>(duration_sequential.count()) /
                   duration_parallel.count();
  printf("Speedup: %.2fx\n\n", speedup);

  // Cleanup
  delete[] image;

  printf("End\n");
  return 0;
}
