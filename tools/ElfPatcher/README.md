# ELF Function Patcher

This script patches an ELF binary by overwriting the machine code of an existing function with custom assembly.
It works by assembling a provided `.S` file into raw bytes and writing those bytes into the correct offset inside the target ELF.

## Usage
`./patcher.sh [OPTIONS] ELF PATCH OFFSET`

### Positional Arguments
- **ELF**: Path to ELF to patch.
- **PATCH**: Path to .S file containing a .text section with the patch.
- **OFFSET**: VA to insert patch at. Obtained via 'obdjump'.

### Options
- `-h`, `--help`: Show this help menu.
- `-v`, `--verbose`: Show verbose output.

## Example
1. Get the VA of the function you want to patch:
    ```bash
    $ objdump -d ./qemu-system-x86_64 | grep rocker_find
    00000000005b8fa0 <rocker_find@@Base>:
    ```
2. Write the code to inject into `inject.S`
    ```asm
    .section .text, "";
    .global inject;
    inject:
        mov (%rax), %rax
    ```
    - Label name does not matter. Everything part of `.text` will be taken
3. Patch the binary
    ```bash
    $ ./patch.sh -v qemu-system-x86_64 patch.S 0x46c010
    ```
    - The patched binary will be saved to `./qemu-system-x86_64_patched`

