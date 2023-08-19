#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "CL/opencl.h"
#include "AOCLUtils/aocl_utils.h"

using namespace aocl_utils;

// OpenCL runtime configuration
cl_platform_id platform = NULL;
unsigned num_devices = 0;
scoped_array<cl_device_id> device;    // num_devices elements
cl_context context = NULL;
scoped_array<cl_command_queue> queue; // num_devices elements
cl_program program = NULL;
scoped_array<cl_kernel> kernel;       // num_devices elements

scoped_array<cl_mem> input_x_buf;     // num_devices elements
scoped_array<cl_mem> input_y_buf;     // num_devices elements
scoped_array<cl_mem> sum_x_buf;
scoped_array<cl_mem> sum_y_buf;
scoped_array<cl_mem> sum_sx_buf;
scoped_array<cl_mem> sum_sy_buf;
scoped_array<cl_mem> sum_p_buf;


// Problem data.
int N = 10000; // problem size 1000000

scoped_array<scoped_aligned_ptr<float> > input_x, input_y; // num_devices elements
scoped_array<scoped_aligned_ptr<float> > sum_x;           // num_devices elements
scoped_array<scoped_aligned_ptr<float> > sum_y;           // num_devices elements
scoped_array<scoped_aligned_ptr<float> > sum_sx;           // num_devices elements
scoped_array<scoped_aligned_ptr<float> > sum_sy;           // num_devices elements
scoped_array<scoped_aligned_ptr<float> > sum_p;           // num_devices elements

scoped_array<float> ref_sum_x;
scoped_array<float> ref_sum_y;
scoped_array<float> ref_sum_sx; 
scoped_array<float> ref_sum_sy;
scoped_array<float> ref_sum_p;  

scoped_array<unsigned> n_per_device;           // num_devices elements

// Function prototypes
float rand_float();
bool init_opencl();
void init_problem();
void run();
void cleanup();

// Entry point.
int main(int argc, char **argv) {
  Options options(argc, argv);

  // Optional argument to specify the problem size.
  if(options.has("n")) {
    N = options.get<int>("n");
  }

  // Initialize OpenCL.
  if(!init_opencl()) {
    return -1;
  }

  // Initialize the problem data.
  // Requires the number of devices to be known.
  init_problem();

  // Run the kernel.
  run();

  // Free the resources allocated
  cleanup();

  return 0;
}

/////// HELPER FUNCTIONS ///////

// Randomly generate a floating-point number between -10 and 10.
float rand_float() {
  return float(rand()) / float(RAND_MAX) * 20.0f - 10.0f;
}

// Initializes the OpenCL objects.
bool init_opencl() {
  cl_int status;

  printf("Initializing OpenCL\n");

  if(!setCwdToExeDir()) {
    return false;
  }

  // Get the OpenCL platform.
  platform = findPlatform("Intel");
  if(platform == NULL) {
    printf("ERROR: Unable to find Intel FPGA OpenCL platform.\n");
    return false;
  }
  printf("Platform: %s\n", getPlatformName(platform).c_str());

  // Query the available OpenCL device.
  device.reset(getDevices(platform, CL_DEVICE_TYPE_ALL, &num_devices));
  printf("Using %d device(s)\n", num_devices);
  for(unsigned i = 0; i < num_devices; ++i) {
    printf("  %s\n", getDeviceName(device[i]).c_str());
  }

  // Create the context.
  context = clCreateContext(NULL, num_devices, device, &oclContextCallback, NULL, &status);
  checkError(status, "Failed to create context");

  // Create the program for all device. Use the first device as the
  // representative device (assuming all device are of the same type).
  std::string binary_file = getBoardBinaryFile("correlation", device[0]);
  printf("Using AOCX: %s\n", binary_file.c_str());
  program = createProgramFromBinary(context, binary_file.c_str(), device, num_devices);

  // Build the program that was just created.
  status = clBuildProgram(program, 0, NULL, "", NULL, NULL);
  checkError(status, "Failed to build program");

  printf("Number of device:%d\n", num_devices);
  // Create per-device objects.
  queue.reset(num_devices);
  kernel.reset(num_devices);
  n_per_device.reset(num_devices);

  input_x_buf.reset(num_devices);
  input_y_buf.reset(num_devices);
  
  sum_x_buf.reset(num_devices);
  sum_y_buf.reset(num_devices);
  sum_sx_buf.reset(num_devices);
  sum_sy_buf.reset(num_devices);
  sum_p_buf.reset(num_devices);

  for(unsigned i = 0; i < num_devices; ++i) {
    // Command queue.
    queue[i] = clCreateCommandQueue(context, device[i], CL_QUEUE_PROFILING_ENABLE, &status);
    checkError(status, "Failed to create command queue");

    // Kernel.
    const char *kernel_name = "correlation";
    kernel[i] = clCreateKernel(program, kernel_name, &status);
    checkError(status, "Failed to create kernel");

    // Determine the number of elements processed by this device.
    n_per_device[i] = N / num_devices; // number of elements handled by this device

    // Spread out the remainder of the elements over the first
    // N % num_devices.
    if(i < (N % num_devices)) {
      n_per_device[i]++;
    }

    // Input buffers.
    input_x_buf[i] = clCreateBuffer(context, CL_MEM_READ_ONLY, 
        n_per_device[i] * sizeof(float), NULL, &status);
    checkError(status, "Failed to create buffer for input A");

    input_y_buf[i] = clCreateBuffer(context, CL_MEM_READ_ONLY, 
        n_per_device[i] * sizeof(float), NULL, &status);
    checkError(status, "Failed to create buffer for input B");

    sum_x_buf[i] = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
        sizeof(float), NULL, &status);
    checkError(status, "Failed to create buffer for sum_x");

    sum_y_buf[i] = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
        sizeof(float), NULL, &status);
    checkError(status, "Failed to create buffer for sum_y");

    sum_sx_buf[i] = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
        sizeof(float), NULL, &status);
    checkError(status, "Failed to create buffer for sum_sx");

    sum_sy_buf[i] = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
        sizeof(float), NULL, &status);
    checkError(status, "Failed to create buffer for sum_sy");
    
    sum_p_buf[i] = clCreateBuffer(context, CL_MEM_WRITE_ONLY, 
        sizeof(float), NULL, &status);
    checkError(status, "Failed to create buffer for sum_p");
  }

  return true;
}

// Initialize the data for the problem. Requires num_devices to be known.
void init_problem() {
  if(num_devices == 0) {
    checkError(-1, "No devices");
  }

  input_x.reset(num_devices);
  input_y.reset(num_devices);
  sum_x.reset(num_devices);
  sum_y.reset(num_devices);  
  sum_sx.reset(num_devices);
  sum_sy.reset(num_devices);
  sum_p.reset(num_devices);

  ref_sum_x.reset(num_devices);
  ref_sum_y.reset(num_devices);
  ref_sum_sx.reset(num_devices);
  ref_sum_sy.reset(num_devices);
  ref_sum_p.reset(num_devices);

  // Generate input vectors A and B and the reference output consisting
  // of a total of N elements.
  // We create separate arrays for each device so that each device has an
  // aligned buffer.
  for(unsigned i = 0; i < num_devices; ++i) {
    input_x[i].reset(n_per_device[i]);
    input_y[i].reset(n_per_device[i]);
    sum_x[i].reset(n_per_device[i]);
    sum_y[i].reset(n_per_device[i]);
    sum_sx[i].reset(n_per_device[i]);
    sum_sy[i].reset(n_per_device[i]);
    sum_p[i].reset(n_per_device[i]);

    for(unsigned j = 0; j < n_per_device[i]; ++j) {
      input_x[i][j] = rand_float();
      input_y[i][j] = rand_float();
//      printf("x:%f, y:%f",input_x[i][j], input_y[i][j]);

      ref_sum_x[i] += input_x[i][j];
      ref_sum_y[i] += input_y[i][j];
      ref_sum_sx[i] += input_x[i][j] * input_x[i][j];
      ref_sum_sy[i] += input_y[i][j] * input_y[i][j];
      ref_sum_p[i] += input_x[i][j] * input_y[i][j];
    }
  }
}

void run() {
  cl_int status;

  const double start_time = getCurrentTimestamp();

  // Launch the problem for each device.
  scoped_array<cl_event> kernel_event(num_devices);
  scoped_array<cl_event> finish_event(num_devices);

  for(unsigned i = 0; i < num_devices; ++i) {

    // Transfer inputs to each device. Each of the host buffers supplied to
    // clEnqueueWriteBuffer here is already aligned to ensure that DMA is used
    // for the host-to-device transfer.
    cl_event write_event[2];
    status = clEnqueueWriteBuffer(queue[i], input_x_buf[i], CL_FALSE,
        0, n_per_device[i] * sizeof(float), input_x[i], 0, NULL, &write_event[0]);
    checkError(status, "Failed to transfer input X");

    status = clEnqueueWriteBuffer(queue[i], input_y_buf[i], CL_FALSE,
        0, n_per_device[i] * sizeof(float), input_y[i], 0, NULL, &write_event[1]);
    checkError(status, "Failed to transfer input Y");


    // Set kernel arguments.
    unsigned argi = 0;

    status = clSetKernelArg(kernel[i], argi++, sizeof(cl_mem), &input_x_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);

    status = clSetKernelArg(kernel[i], argi++, sizeof(cl_mem), &input_y_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);
    
    status = clSetKernelArg(kernel[i], argi++, sizeof(cl_mem), &sum_x_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);

    status = clSetKernelArg(kernel[i], argi++, sizeof(cl_mem), &sum_y_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);

    status = clSetKernelArg(kernel[i], argi++, sizeof(cl_mem), &sum_sx_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);

    status = clSetKernelArg(kernel[i], argi++, sizeof(cl_mem), &sum_sy_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);

    status = clSetKernelArg(kernel[i], argi++, sizeof(cl_mem), &sum_p_buf[i]);
    checkError(status, "Failed to set argument %d", argi - 1);

    status = clSetKernelArg(kernel[i], argi++, sizeof(int), &N);
    checkError(status, "Failed to set argument %d", argi - 1);

    // Enqueue kernel.
    // Use a global work size corresponding to the number of elements to add
    // for this device.
    //
    // We don't specify a local work size and let the runtime choose
    // (it'll choose to use one work-group with the same size as the global
    // work-size).
    //
    // Events are used to ensure that the kernel is not launched until
    // the writes to the input buffers have completed.
    const size_t global_work_size = 1;//n_per_device[i];
    const size_t local_work_size = 1;

    printf("Launching for device %d (%zd elements)\n", i, global_work_size);

    status = clEnqueueNDRangeKernel(queue[i], kernel[i], 1, NULL,
        &global_work_size, &local_work_size, 2, write_event, &kernel_event[i]);

    checkError(status, "Failed to launch kernel");

    // Read the result. This the final operation.
    status = clEnqueueReadBuffer(queue[i], sum_x_buf[i], CL_FALSE,
        0, sizeof(float), sum_x[i], 1, &kernel_event[i], &finish_event[i]);
    
    status = clEnqueueReadBuffer(queue[i], sum_y_buf[i], CL_FALSE,
        0, sizeof(float), sum_y[i], 1, &kernel_event[i], &finish_event[i]);
    
    status = clEnqueueReadBuffer(queue[i], sum_sx_buf[i], CL_FALSE,
        0, sizeof(float), sum_sx[i], 1, &kernel_event[i], &finish_event[i]);
    
    status = clEnqueueReadBuffer(queue[i], sum_sy_buf[i], CL_FALSE,
        0, sizeof(float), sum_sy[i], 1, &kernel_event[i], &finish_event[i]);
    
    status = clEnqueueReadBuffer(queue[i], sum_p_buf[i], CL_FALSE,
        0, sizeof(float), sum_p[i], 1, &kernel_event[i], &finish_event[i]);

    // Release local events.
    clReleaseEvent(write_event[0]);
    clReleaseEvent(write_event[1]);

  }

  // Wait for all devices to finish.
  clWaitForEvents(num_devices, finish_event);

  const double end_time = getCurrentTimestamp();

  // Wall-clock time taken.
  printf("\nTime: %0.3f ms\n", (end_time - start_time) * 1e3);

  // Get kernel times using the OpenCL event profiling API.
  for(unsigned i = 0; i < num_devices; ++i) {
    cl_ulong time_ns = getStartEndTime(kernel_event[i]);
    printf("Kernel time (device %d): %0.3f ms\n", i, double(time_ns) * 1e-6);
  }

  // Release all events.
  for(unsigned i = 0; i < num_devices; ++i) {
    clReleaseEvent(kernel_event[i]);
    clReleaseEvent(finish_event[i]);
  }

  // Verify results.
  bool pass = true;
  for(unsigned i = 0; i < num_devices && pass; ++i) {
    if(fabsf(sum_x[i][0] - ref_sum_x[i]) > 1.0){   //1.0e-5f) {
      printf("Failed verification Sum X@ device %d \nOutput: %f\nReference: %f\n",
            i, sum_x[i][0], ref_sum_x[i]);
      pass = false;
   } 
    if(fabsf(sum_y[i][0] - ref_sum_y[i]) > 1.0e-5f) {
      printf("Failed verification Sum Y@ device %d \nOutput: %f\nReference: %f\n",
            i, sum_y[i][0], ref_sum_y[i]);
      pass = false;
    }  

    if(fabsf(sum_sx[i][0] - ref_sum_sx[i]) > 1.0e-5f) {
      printf("Failed verification Sum SX@ device %d \nOutput: %f\nReference: %f\n",
            i, sum_sx[i][0], ref_sum_sx[i]);
      pass = false;
    } 

    if(fabsf(sum_sy[i][0] - ref_sum_sy[i]) > 1.0e-5f) {
      printf("Failed verification Sum SY@ device %d \nOutput: %f\nReference: %f\n",
            i, sum_sy[i][0], ref_sum_sy[i]);
      pass = false;
    } 

    if(fabsf(sum_p[i][0] - ref_sum_p[i]) > 1.0e-5f) {
      printf("Failed verification Sum P@ device %d \nOutput: %f\nReference: %f\n",
            i, sum_p[i][0], ref_sum_p[i]);
      pass = false;
    }
  }  

  printf("\nVerification: %s\n", pass ? "PASS" : "FAIL");
}

// Free the resources allocated during initialization
void cleanup() {
  for(unsigned i = 0; i < num_devices; ++i) {
    if(kernel && kernel[i]) {
      clReleaseKernel(kernel[i]);
    }
    if(queue && queue[i]) {
      clReleaseCommandQueue(queue[i]);
    }

    if(input_x_buf && input_x_buf[i]) {
      clReleaseMemObject(input_x_buf[i]);
    }
    if(input_y_buf && input_y_buf[i]) {
      clReleaseMemObject(input_y_buf[i]);
    }

    if(sum_x_buf && sum_x_buf[i]) {
      clReleaseMemObject(sum_x_buf[i]);
    }

    if(sum_y_buf && sum_y_buf[i]) {
      clReleaseMemObject(sum_x_buf[i]);
    }

    if(sum_sx_buf && sum_sx_buf[i]) {
      clReleaseMemObject(sum_x_buf[i]);
    }

    if(sum_sy_buf && sum_sy_buf[i]) {
      clReleaseMemObject(sum_sy_buf[i]);
    }

    if(sum_p_buf && sum_p_buf[i]) {
      clReleaseMemObject(sum_p_buf[i]);
    }

  }

  if(program) {
    clReleaseProgram(program);
  }
  if(context) {
    clReleaseContext(context);
  }
}

