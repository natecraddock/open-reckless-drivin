"""Generate a Reckless Drivin' license code from a name"""
import sys

# This is the global decryption key used in the Reckless Drivin' source code.
# It is derived from a valid player's registration name and code.
G_KEY = 0x1E42A71F

def str_to_hex(string: str) -> list:
    return [hex(ord(c)) for c in list(string)]

def str_to_num(string: str) -> int:
    return int("".join([str(h)[2:] for h in str_to_hex(string)]), base=16)

def gen_code(name: str) -> str:
    # Valid names must be at least 4 characters long
    if len(name) < 4:
        sys.stderr.write("Valid names must be at least 4 characters long\n")
        sys.exit(1)

    # Convert to all-caps no spaces
    name = name.upper().replace(" ", "")

    # XOR the GKEY with the ascii representation of the last four chars of the name
    name_num = str_to_num(name[-4:])
    code_num = G_KEY ^ name_num
    
    # Now create a seed based on the first four letters of the name
    # This is an arbitrary decision. I am not certain how the original
    # registration code generator worked.
    seed = sum([int(h, base=16) for h in str_to_hex(name[:4])]) % 0xFF

    seed_mask = seed + (seed << 8) + (seed << 16) + (seed << 24)
    code_first_8 = code_num ^ seed_mask

    # The code is the 8 bytes joined with the mask both in hex
    code_first_8 = hex(code_first_8)[2:]
    code_last_2 = hex(seed)[2:]

    # Prefix with a zero if needed
    if len(code_first_8) < 8:
        code_first_8 = "0" + code_first_8
    if len(code_last_2) < 2:
        code_last_2 = "0" + code_last_2

    return (code_first_8 + code_last_2).upper()

if len(sys.argv) != 2:
    sys.stderr.write(f"Usage: {sys.argv[0]} name\n")
    sys.exit(1)
    
print(gen_code(sys.argv[1]))
