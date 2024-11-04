# p2p-encryption
Peer-to-peer encrypted chat client

## Compilation
Run `make` in the main directory

## Running
Run `./bin/bn <arg>` with one of the following arguments
- `keygen` - generates `key` and `key.pub` - Note, this will take ~30s to 1min
- `encrypt` - prompts you to enter a string, then returns the encrypted string
- `decrypt` - prompts you to enter an encrypted string, then returns the plaintext string
- `prime` - generates a 256-bit prime number

## Bugs
If you find any bugs, please contact s.kollar@student.unsw.edu.au
