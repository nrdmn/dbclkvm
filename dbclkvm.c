#ifndef __loongarch__
#error "This must be compiled for loongarch"
#endif

#include <fcntl.h>
#include <linux/kvm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
  int ret = EXIT_SUCCESS;

  int kvm = open("/dev/kvm", 0);
  if (kvm == -1) {
    perror("Failed to open /dev/kvm");
    ret = EXIT_FAILURE;
    goto fail_open_kvm;
  }

  int api_version = ioctl(kvm, KVM_GET_API_VERSION, 0);
  if (api_version == -1) {
    perror("Failed to KVM_GET_API_VERSION");
    ret = EXIT_FAILURE;
    goto fail_get_api_version;
  }
  if (api_version != 12) {
    fprintf(stderr, "Unsupported KVM API version, expected 12, got %d\n",
        api_version);
    ret = EXIT_FAILURE;
    goto fail_unsupported_api_version;
  }

  int vm = ioctl(kvm, KVM_CREATE_VM, 0);
  if (vm == -1) {
    perror("Failed to KVM_CREATE_VM");
    ret = EXIT_FAILURE;
    goto fail_create_vm;
  }

  const int vcpu_id = 0;
  int vcpu = ioctl(vm, KVM_CREATE_VCPU, vcpu_id);
  if (vcpu == -1) {
    perror("Failed to KVM_CREATE_VCPU");
    ret = EXIT_FAILURE;
    goto fail_create_vcpu;
  }

  const size_t memory_size = 0x10000;
  unsigned char *memory = mmap(0, memory_size,
      PROT_READ|PROT_WRITE|PROT_EXEC, MAP_SHARED|MAP_ANONYMOUS, -1, 0);
  if (memory == MAP_FAILED) {
    perror("Failed to allocate memory for VM");
    ret = EXIT_FAILURE;
    goto fail_mmap_memory;
  }

  const uint32_t idle_0 = 0x06488000;
  const uint32_t dbcl_0 = 0x002a8000;
  for (size_t i = 0; i < memory_size; i += sizeof(idle_0)) {
    memcpy(memory + i, &idle_0, sizeof(idle_0));
  }
  memcpy(memory, &dbcl_0, sizeof(dbcl_0));

  struct kvm_userspace_memory_region region = {
    .slot = 0,
    .guest_phys_addr = 0,
    .memory_size = memory_size,
    .userspace_addr = (uintptr_t)memory,
  };

  if (ioctl(vm, KVM_SET_USER_MEMORY_REGION, &region) == -1) {
    perror("Failed to KVM_SET_USER_MEMORY_REGION");
    ret = EXIT_FAILURE;
    goto fail_set_user_memory_region;
  }

  if (ioctl(vcpu, KVM_RUN, 0) == -1) {
    perror("Failed to KVM_RUN");
    ret = EXIT_FAILURE;
    goto fail_run;
  }

  puts("Success");

fail_run:
fail_set_user_memory_region:
  munmap(memory, memory_size);
fail_mmap_memory:
  close(vcpu);
fail_create_vcpu:
  close(vm);
fail_unsupported_api_version:
fail_get_api_version:
fail_create_vm:
  close(kvm);
fail_open_kvm:

  return ret;
}
