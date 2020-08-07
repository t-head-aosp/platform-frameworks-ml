/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-base/logging.h>
#include <android-base/mapped_file.h>
#include <cutils/ashmem.h>
#include <cutils/native_handle.h>
#include <sys/mman.h>

#include <limits>
#include <memory>
#include <utility>

#include "Result.h"
#include "SharedMemory.h"
#include "TypeUtils.h"
#include "Types.h"

namespace android::nn {
namespace {

GeneralResult<Mapping> mapAshmem(const Memory& memory) {
    CHECK_LE(memory.size, std::numeric_limits<uint32_t>::max());
    const auto size = memory.size;

    int fd = memory.handle->handle()->data[0];
    std::shared_ptr<base::MappedFile> mapping =
            base::MappedFile::FromFd(fd, /*offset=*/0, size, PROT_READ | PROT_WRITE);
    if (mapping == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Can't mmap the file descriptor.";
    }
    void* data = mapping->data();

    return Mapping{.pointer = data, .size = size, .context = std::move(mapping)};
}

struct MmapFdMappingContext {
    int prot;
    std::any context;
};

GeneralResult<Mapping> mapMemFd(const Memory& memory) {
    const size_t size = memory.size;
    const native_handle_t* handle = memory.handle->handle();
    const int fd = handle->data[0];
    const int prot = handle->data[1];
    const size_t offset = getOffsetFromInts(handle->data[2], handle->data[3]);

    std::shared_ptr<base::MappedFile> mapping = base::MappedFile::FromFd(fd, offset, size, prot);
    if (mapping == nullptr) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Can't mmap the file descriptor.";
    }
    void* data = mapping->data();

    auto context = MmapFdMappingContext{.prot = prot, .context = std::move(mapping)};
    return Mapping{.pointer = data, .size = size, .context = std::move(context)};
}

}  // namespace

GeneralResult<Memory> createSharedMemory(size_t size) {
    int fd = ashmem_create_region("NnapiAshmem", size);
    if (fd < 0) {
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE)
               << "ashmem_create_region(" << size << ") fails with " << fd;
    }

    native_handle_t* handle = native_handle_create(1, 0);
    if (handle == nullptr) {
        // TODO(b/120417090): is ANEURALNETWORKS_UNEXPECTED_NULL the correct error to return here?
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to create native_handle";
    }
    handle->data[0] = fd;

    // Create a NativeHandle which owns the native handle and fd so that we don't have to manually
    // clean either the native handle or the fd.
    auto nativeHandle = ::android::NativeHandle::create(handle, /*ownsHandle=*/true);

    return Memory{.handle = std::move(nativeHandle), .size = size, .name = "ashmem"};
}

GeneralResult<Memory> createSharedMemoryFromFd(size_t size, int prot, int fd, size_t offset) {
    if (size == 0 || fd < 0) {
        return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Invalid size or fd";
    }

    // Duplicate the file descriptor so the resultant Memory owns its own version.
    int dupfd = dup(fd);
    if (dupfd == -1) {
        // TODO(b/120417090): is ANEURALNETWORKS_UNEXPECTED_NULL the correct error to return here?
        return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Failed to dup the fd";
    }

    // Create a temporary native handle to own the dupfd.
    native_handle_t* nativeHandle = native_handle_create(1, 3);
    if (nativeHandle == nullptr) {
        close(dupfd);
        // TODO(b/120417090): is ANEURALNETWORKS_UNEXPECTED_NULL the correct error to return here?
        return NN_ERROR(ErrorStatus::GENERAL_FAILURE) << "Failed to create native_handle";
    }

    const auto [lowOffsetBits, highOffsetBits] = getIntsFromOffset(offset);
    nativeHandle->data[0] = dupfd;
    nativeHandle->data[1] = prot;
    nativeHandle->data[2] = lowOffsetBits;
    nativeHandle->data[3] = highOffsetBits;

    // Create a NativeHandle which owns the native handle and fd so that we don't have to manually
    // clean either the native handle or the fd.
    auto ownedHandle = ::android::NativeHandle::create(nativeHandle, /*ownsHandle=*/true);

    return Memory{.handle = std::move(ownedHandle), .size = size, .name = "mmap_fd"};
}

GeneralResult<Memory> createSharedMemoryFromHidlMemory(const hardware::hidl_memory& /*memory*/) {
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "hidl_memory not supported on host";
}

GeneralResult<Memory> createSharedMemoryFromAHWB(const AHardwareBuffer& /*ahwb*/) {
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT)
           << "AHardwareBuffer memory not supported on host";
}

GeneralResult<Mapping> map(const Memory& memory) {
    if (memory.name == "ashmem") {
        return mapAshmem(memory);
    }
    if (memory.name == "mmap_fd") {
        return mapMemFd(memory);
    }
    return NN_ERROR(ErrorStatus::INVALID_ARGUMENT) << "Cannot map unknown memory " << memory.name;
}

bool flush(const Mapping& mapping) {
    if (const auto* mmapFdMapping = std::any_cast<MmapFdMappingContext>(&mapping.context)) {
        if (!std::holds_alternative<void*>(mapping.pointer)) {
            return true;
        }
        void* data = std::get<void*>(mapping.pointer);
        const int prot = mmapFdMapping->prot;
        if (prot & PROT_WRITE) {
            const size_t size = mapping.size;
            return msync(data, size, MS_SYNC) == 0;
        }
    }
    // No-op for other types of memory.
    return true;
}

}  // namespace android::nn
