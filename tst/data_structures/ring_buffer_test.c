#include "data_structures/ring_buffer.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "logging/log_utils.h"
#include "pitest/api.h"
#include "pitest/api/assertions.h"
#include "util.h"

static int test_simple_write();
static int test_simple_read_peek();
static int test_write_more_than_free();
static int test_simple_wraparound();
static int test_clear_full_buffer();

static ssize_t rb_read_until_completion(int fd, size_t count, pictrl_rb_t *rb,
                                        pictrl_read_flag flag);
static ssize_t rb_write_until_completion(int fd, size_t count, pictrl_rb_t *rb);
static size_t read_from_test_file(uint8_t *data, size_t count);
static size_t write_to_test_file(uint8_t *data, size_t count);

#define RING_BUF_SIZE (size_t)8
#define TEST_FILE_TEMPLATE_PREFIX "./ring_buffer_testXXXXXX"
#define TEST_FILE_TEMPLATE_SUFFIX ".tmp"
#define TEST_FILE_TEMPLATE_SUFFIX_LEN \
  (int)(sizeof(TEST_FILE_TEMPLATE_SUFFIX) / sizeof(char) - 1)
static char test_file_name[] =
    TEST_FILE_TEMPLATE_PREFIX TEST_FILE_TEMPLATE_SUFFIX;

// Fixtures
static FILE *test_file = NULL;
static pictrl_rb_t ring_buffer;

int before_all() {
  // Create temp file
  const int test_fd = mkstemps(test_file_name, TEST_FILE_TEMPLATE_SUFFIX_LEN);
  if (test_fd < 0) {
    pictrl_log_error("Error creating temp file %s: %s\n", test_file_name,
                     strerror(errno));
    return -1;
  }

  // Open file, use as fixture
  FILE *test_fp = fdopen(test_fd, "w+");
  if (test_fp == NULL) {
    pictrl_log_error("Error getting FILE pointer for open file %s: %s\n",
                     test_file_name, strerror(errno));
    return -1;
  }
  test_file = test_fp;
  pictrl_log_debug("Created temp file %s\n", test_file_name);

  // Initialize ring buffer
  if (pictrl_rb_init(&ring_buffer, RING_BUF_SIZE) == NULL) {
    pictrl_log_error("Could not initialize ring buffer\n");
    return -1;
  }
  pictrl_log_debug("Initialized ring buffer to %zu bytes\n", RING_BUF_SIZE);

  return 0;
}

int before_each() {
  // Clear out file
  rewind(test_file);
  if (ftruncate(fileno(test_file), 0) < 0) {
    pictrl_log_error("Error truncating temp file: %s\n", strerror(errno));
    return -1;
  }
  pictrl_log_debug("Truncated temp file\n");

  // Clear ring buffer
  pictrl_rb_clear(&ring_buffer);
  return 0;
}

int after_all() {
  // Close test file
  int ret = fclose(test_file);
  if (ret != 0) {
    pictrl_log_error("Was not able to close temp file %s: %s\n", test_file_name,
                     strerror(errno));
    return ret;
  }
  test_file = NULL;

  // Remove file
  ret = unlink(test_file_name);
  if (ret < 0) {
    pictrl_log_warn("Could not remove temp file %s: %s\n", test_file_name,
                    strerror(errno));  // Error + return?
  } else {
    pictrl_log_debug("Removed temp file %s\n", test_file_name);
  }

  // Destroy ring buffer
  pictrl_rb_destroy(&ring_buffer);
  return ret;
}

int main() {
  const TestCase test_cases[] = {
      {
          .test_name = "Simple write",
          .test_function = &test_simple_write,
      },
      {
          .test_name = "Simple read (peek)",
          .test_function = &test_simple_read_peek,
      },
      {
          .test_name = "Write more than free",
          .test_function = &test_write_more_than_free,
      },
      {
          .test_name = "Simple wraparound",
          .test_function = &test_simple_wraparound,
      },
      {
          .test_name = "Clear full buffer",
          .test_function = &test_clear_full_buffer,
      }};

  const TestSuite suite = {
      .name = "Ring buffer tests",
      .test_cases = test_cases,
      .num_tests = PICTRL_SIZE(test_cases),
      .before_after_all = {.setup = &before_all, .teardown = &after_all},
      .before_after_each = {.setup = &before_each, .teardown = NULL}};

  return run_test_suite(&suite);
}

static int test_simple_write() {
  // Arrange
  uint8_t data[] = {4, 9, 5, 6, 1};

  const size_t num_bytes_to_write = sizeof(data);
  if (write_to_test_file(data, num_bytes_to_write) < num_bytes_to_write) {
    return 1;
  }
  rewind(test_file);
  pictrl_log_debug("Wrote all %zu bytes of test data to temp file\n",
                   num_bytes_to_write);

  // Act
  const int test_fd = fileno(test_file);
  if (rb_write_until_completion(test_fd, num_bytes_to_write, &ring_buffer) !=
      (ssize_t)num_bytes_to_write) {
    // pictrl_log_error("Error writing to ring buffer\n");
    return 2;
  }
  pictrl_log_debug("Wrote %zu bytes to ring buffer\n", num_bytes_to_write);

  // Assert
  if (!array_equals(ring_buffer.buffer, ring_buffer.num_items, data, num_bytes_to_write)) {
    // TODO: Make these logs all go to stderr.. d'oh
    pictrl_log_error("Data mismatch. Expected data: ");
    print_buf(data, num_bytes_to_write);
    pictrl_log_error("Received: ");
    print_buf(ring_buffer.buffer, num_bytes_to_write);
    return 3;
  }
  pictrl_log_debug("Data matches!\n");

  return 0;
}

static int test_simple_read_peek() {
  // Arrange
  uint8_t orig_data[] = {1, 2, 3, 4, 5, 6, 7};
  const size_t num_bytes_to_read = sizeof(orig_data);

  // Populate ring buffer
  memcpy(ring_buffer.buffer, orig_data, num_bytes_to_read);
  ring_buffer.num_items = num_bytes_to_read;
  pictrl_log_debug("Populated ring buffer with %zu bytes of original data\n",
                   num_bytes_to_read);

  // Act
  const int test_fd = fileno(test_file);
  if (rb_read_until_completion(test_fd, num_bytes_to_read, &ring_buffer,
                               PICTRL_READ_PEEK) != num_bytes_to_read) {
    pictrl_log_error("Error reading from ring buffer\n");
    return 2;
  }
  rewind(test_file);
  pictrl_log_debug("Read all %zu bytes from ring buffer\n", num_bytes_to_read);

  // Assert
  uint8_t read_data[sizeof(orig_data)] = {0};
  if (read_from_test_file(read_data, num_bytes_to_read) < num_bytes_to_read) {
    return 2;
  }

  if (!array_equals(orig_data, num_bytes_to_read, read_data,
                    num_bytes_to_read)) {
    pictrl_log_error(
        "Read data does not match original data.\nExpected data: ");
    print_buf(orig_data, num_bytes_to_read);
    pictrl_log_error("Received: ");
    print_buf(read_data, num_bytes_to_read);
    return 3;
  }

  if (!array_equals(orig_data, num_bytes_to_read, ring_buffer.buffer,
                    num_bytes_to_read)) {
    pictrl_log_error("Ring buffer data was somehow modified.\nExpected data: ");
    print_buf(orig_data, num_bytes_to_read);
    print_ring_buffer(&ring_buffer);
    return 3;
  }

  pictrl_log_debug("Data matches!\n");
  return 0;
}

int test_write_more_than_free() {
  // Arrange
  uint8_t orig_data[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
  const size_t num_bytes_to_write = sizeof(orig_data);

  if (write_to_test_file(orig_data, num_bytes_to_write) < num_bytes_to_write) {
    return 1;
  }
  rewind(test_file);
  pictrl_log_debug("Wrote test data to temp file\n");

  // Act
  const int test_fd = fileno(test_file);
  size_t num_bytes_written =
      rb_write_until_completion(test_fd, num_bytes_to_write, &ring_buffer);
  if (errno != ENOBUFS) {
    pictrl_log_error(
        "Expected errno to be set to ENOBUFS (%d), but errno is %d\n", ENOBUFS,
        errno);
    return 1;
  }
  if (num_bytes_written !=
      ring_buffer.capacity) {  // We should've capped out at the rb's capacity
    pictrl_log_error(
        "Expected to write %zu bytes to ring buffer, but %zu bytes were "
        "somehow written\n",
        ring_buffer.capacity, num_bytes_written);
    return 2;
  }
  pictrl_log_debug("Wrote all %zu bytes to ring buffer\n", num_bytes_written);

  // Assert
  if (!array_equals(ring_buffer.buffer, ring_buffer.capacity, orig_data, RING_BUF_SIZE)) {
    // TODO: Make these logs all go to stderr.. d'oh
    pictrl_log_error("Data mismatch. Expected data: ");
    print_buf(orig_data, RING_BUF_SIZE);
    pictrl_log_error("Received: ");
    print_buf(ring_buffer.buffer, RING_BUF_SIZE);
    return 3;
  }
  // TODO: Check that rest of array is in the file
  pictrl_log_debug("Data matches!\n");
  return 0;
}

int test_simple_wraparound() {
  // Arrange
  uint8_t orig_data[] = {1, 2, 3, 4, 5, 6, 7, 8};

  if (write_to_test_file(orig_data, RING_BUF_SIZE) < RING_BUF_SIZE) {
    return 1;
  }
  rewind(test_file);
  pictrl_log_debug("Wrote test data to temp file\n");

  // Act
  ring_buffer.data_start += RING_BUF_SIZE - 1;
  const int test_fd = fileno(test_file);
  const ssize_t num_bytes_written =
      rb_write_until_completion(test_fd, RING_BUF_SIZE, &ring_buffer);
  if (num_bytes_written != RING_BUF_SIZE) {
    pictrl_log_error(
        "Expected to write %zu bytes, but %zd bytes were somehow written\n",
        RING_BUF_SIZE, num_bytes_written);
    return 2;
  }
  pictrl_log_debug(
      "Wrote all %zd bytes to ring buffer, starting at the last position in "
      "the internal buffer\n",
      num_bytes_written);

  // Assert
  uint8_t expected_data[] = {2, 3, 4, 5, 6, 7, 8, 1};
  if (!array_equals(ring_buffer.buffer, ring_buffer.capacity, expected_data, ring_buffer.capacity)) {
    // TODO: Make these logs all go to stderr.. d'oh
    pictrl_log_error("Data mismatch. Expected data: ");
    print_buf(expected_data, RING_BUF_SIZE);
    pictrl_log_error("Received: ");
    print_buf(ring_buffer.buffer, RING_BUF_SIZE);

    return 3;
  }
  pictrl_log_debug("Data matches!\n");
  return 0;
}

int test_clear_full_buffer() {
  // Arrange
  uint8_t orig_data[] = {0, 1, 2, 3, 4, 5, 6, 7};

  // Populate ring buffer
  memcpy(ring_buffer.buffer, orig_data, RING_BUF_SIZE);
  ring_buffer.num_items = RING_BUF_SIZE;
  pictrl_log_debug("Populated ring buffer with original data\n");

  // Act
  pictrl_rb_clear(&ring_buffer);
  pictrl_log_debug("Cleared ring buffer\n");

  // Assert
  uint8_t expected_data[sizeof(orig_data)] = {0};
  if (!array_equals(ring_buffer.buffer, ring_buffer.capacity, expected_data, RING_BUF_SIZE)) {
    // TODO: Make these logs all go to stderr.. d'oh
    pictrl_log_error("Data mismatch. Expected data: ");
    print_buf(expected_data, RING_BUF_SIZE);
    pictrl_log_error("Received: ");
    print_buf(ring_buffer.buffer, RING_BUF_SIZE);
    return 3;
  } else if (ring_buffer.num_items != 0) {
    pictrl_log_error("Expected num_items to be 0. Received: %zu", ring_buffer.num_items);
    return 4;
  } else if (ring_buffer.data_start != 0) {
    pictrl_log_error("Expected data_start to be 0. Received: %zu",
                     ring_buffer.data_start);
    return 5;
  }

  pictrl_log_debug("Ring buffer is empty as expected\n");
  return 0;
}

// These are surely not thread-safe
static ssize_t rb_read_until_completion(int fd, size_t count, pictrl_rb_t *rb,
                                        pictrl_read_flag flag) {
  const bool reading_more_than_avail = count > rb->num_items;

  size_t bytes_read = 0;
  while (bytes_read < count) {
    const size_t bytes_left = count - bytes_read;
    const bool no_data_left = rb->num_items <= bytes_left;
    const ssize_t num_read = pictrl_rb_read(fd, bytes_left, rb, flag);
    if (num_read < 0) {
      int err = errno;
      if (!reading_more_than_avail || !no_data_left) {
        pictrl_log_error("Error reading from temp file: %s\n", strerror(err));
        errno = err;
        return -1;
      }
      return bytes_read;
    }
    bytes_read += (size_t)num_read;
  }
  return (ssize_t)bytes_read;
}

static ssize_t rb_write_until_completion(int fd, size_t count,
                                         pictrl_rb_t *rb) {
  const bool inserting_more_than_avail = count > (rb->capacity - rb->num_items);

  size_t bytes_written = 0;
  while (bytes_written < count) {
    const size_t bytes_left = count - bytes_written;
    const bool is_rb_full = (rb->capacity - rb->num_items) <= bytes_left;

    const ssize_t written = pictrl_rb_write(fd, bytes_left, rb);
    if (written < 0) {
      int err = errno;
      if (!inserting_more_than_avail || !is_rb_full) {
        pictrl_log_error("Error writing to temp file: %s\n", strerror(err));
        errno = err;
        return -1;
      }
      return bytes_written;
    }
    if (written == 0) {
      return bytes_written;
    }
    bytes_written += (size_t)written;
  }
  return (ssize_t)bytes_written;
}

static size_t read_from_test_file(uint8_t *data, size_t count) {
  const size_t num_read = fread(data, sizeof(data[0]), count, test_file);
  if (num_read < count) {
    // Should prob log the exact error...
    pictrl_log_error(
        "Error reading %zu bytes from test file... read %zu bytes instead\n",
        count, num_read);
    return 0;
  }
  return num_read;
}

static size_t write_to_test_file(uint8_t *data, size_t count) {
  const size_t num_written = fwrite(data, sizeof(data[0]), count, test_file);
  if (num_written < count) {
    // Should prob log the exact error...
    pictrl_log_error(
        "Error writing %zu bytes to test file... wrote %zu bytes instead\n",
        count, num_written);
    return 0;
  }
  return num_written;
}
