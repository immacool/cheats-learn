import pymem

astroneer = pymem.Pymem("Astro-Win64-Shipping.exe")

p0 = astroneer.process_base.lpBaseOfDll + 0x049CE158
offsets = [0x58, 0x658, 0x8, 0x30, 0x18, 0x60, 0xB0]

# Get the address of the pointer chain
def get_address(base, offsets):
    address = base
    for offset in offsets:
        address = astroneer.read_int(address) + offset
    return address

# Get the value at the address
val = astroneer.read_double(get_address(p0, offsets))

print(f'Value at address: {val}')