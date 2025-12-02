#include <omp.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <limits>
#include <random>

struct node_t {
  node_t* left{};
  node_t* right{};
  float value{};
};

// Sequential find_min for verification
float find_min_sequential(node_t const* const node) {
  if (!node) return std::numeric_limits<float>::max();

  float min_left = find_min_sequential(node->left);
  float min_right = find_min_sequential(node->right);

  return std::min(node->value, std::min(min_left, min_right));
}

float find_min_internal(node_t const* const node, int depth = 0) {
  node_t const* const node_left = node->left;
  node_t const* const node_right = node->right;
  float min_left = std::numeric_limits<float>::max();
  float min_right = std::numeric_limits<float>::max();

  if (depth > 10) {
    if (node_left) {
#pragma omp task shared(node_left, min_left) default(none)
      min_left = find_min_internal(node_left);
    }
    if (node_right) {
#pragma omp task shared(node_right, min_right) default(none)
      min_right = find_min_internal(node_right);
    }
#pragma omp taskwait
  } else {
    min_left = min_right = find_min_sequential(node);
  }

  return std::min(min_right, std::min(min_left, node->value));
}

float find_min(node_t const* const root) {
  float min;
#pragma omp parallel firstprivate(root) shared(min) default(none)
  {
#pragma omp single
    min = find_min_internal(root);
  }
  return min;
}

// Generate a random binary tree
node_t* generate_tree(int depth, std::mt19937& gen,
                      std::uniform_real_distribution<float>& dist) {
  if (depth <= 0) {
    return nullptr;
  }

  node_t* node = new node_t();
  node->value = dist(gen);
  node->left = generate_tree(depth - 1, gen, dist);
  node->right = generate_tree(depth - 1, gen, dist);

  return node;
}

// Free the tree memory
void free_tree(node_t* node) {
  if (!node) return;
  free_tree(node->left);
  free_tree(node->right);
  delete node;
}

int main() {
  printf("Start\n");

  // Generate a random tree
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<float> dist(0.0f, 1000.0f);

  int tree_depth = 20;
  printf("Generating tree with depth %d...\n", tree_depth);
  node_t* root = generate_tree(tree_depth, gen, dist);

  // Find minimum using parallel version
  printf("Finding minimum (parallel)...\n");
  auto start_parallel = std::chrono::high_resolution_clock::now();
  float min_parallel = find_min(root);
  auto end_parallel = std::chrono::high_resolution_clock::now();
  auto duration_parallel =
      std::chrono::duration_cast<std::chrono::microseconds>(end_parallel -
                                                            start_parallel);
  printf("Parallel minimum: %.2f\n", min_parallel);
  printf("Parallel time: %ld μs (%.3f ms)\n\n", duration_parallel.count(),
         duration_parallel.count() / 1000.0);

  // Verify with sequential version
  printf("Finding minimum (sequential)...\n");
  auto start_sequential = std::chrono::high_resolution_clock::now();
  float min_sequential = find_min_sequential(root);
  auto end_sequential = std::chrono::high_resolution_clock::now();
  auto duration_sequential =
      std::chrono::duration_cast<std::chrono::microseconds>(end_sequential -
                                                            start_sequential);
  printf("Sequential minimum: %.2f\n", min_sequential);
  printf("Sequential time: %ld μs (%.3f ms)\n\n", duration_sequential.count(),
         duration_sequential.count() / 1000.0);

  // Verify results match
  if (min_parallel == min_sequential) {
    printf("✓ Results match!\n");
  } else {
    printf("✗ Results differ!\n");
  }

  // Cleanup
  free_tree(root);

  printf("End\n");
  return 0;
}
