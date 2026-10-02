# Linux Device Driver Concepts

## 1. Overview

This project is a user-space Linux/C++ simulator that models
NVMe-style storage behavior. It incorporates several concepts
commonly associated with Linux device-driver and storage systems.

## 2. Device Abstraction

The `NVMeDevice` class represents the modeled storage device.

It provides an abstraction between the write-through cache and
the underlying simulated NVMe queues.

## 3. Request Submission

Write requests are submitted through:

`NVMeDevice::submit_write()`

This models the process of submitting an I/O request to a storage
device.

## 4. Queue Management

The project models multiple NVMe queue pairs using
`NVMeQueuePair`.

Each queue maintains pending write commands and processes them
through a worker thread.

## 5. Request Completion

C++ `std::promise` and `std::future` are used to model asynchronous
request completion.

The submitting component can wait for the corresponding operation
to complete.

## 6. Multi-Queue I/O

The optimized implementation distributes requests across multiple
queues using round-robin scheduling.

This models the multi-queue architecture commonly used by modern
storage devices.

## 7. Block I/O

The simulator uses:

- Logical Block Address (LBA)
- Fixed block size
- Block-based offsets

For this project, the block size is 4096 bytes.

## 8. Linux Storage APIs

The simulator uses Linux system-level file I/O:

- `pwrite()`
- `O_DIRECT`
- `posix_memalign()`

`O_DIRECT` and aligned buffers are used to model direct storage
access more closely.

## 9. Request Processing

The optimized cache additionally models:

- write coalescing
- write batching
- background flushing
- multi-queue dispatch

These mechanisms allow multiple logical application writes to be
combined into fewer physical write operations.

## 10. Kernel Module Status

A loadable Linux kernel module (`.ko`) has not been implemented
in the current version.

The current project is a user-space Linux/C++ simulator focused on
understanding storage architecture, system programming, request
queues, and I/O behavior.

## 11. Concept-to-Implementation Mapping

| Linux/Storage Concept | Project Implementation |
|---|---|
| Device abstraction | `NVMeDevice` |
| Request submission | `submit_write()` |
| Queue management | `NVMeQueuePair` |
| Request completion | `std::promise` / `std::future` |
| Multi-queue I/O | Multiple queue pairs |
| Queue scheduling | Round-robin dispatch |
| Block I/O | LBA + 4096-byte blocks |
| Linux file I/O | `pwrite()` |
| Direct I/O | `O_DIRECT` |
| Aligned buffers | `posix_memalign()` |
| Kernel module | Not implemented |
