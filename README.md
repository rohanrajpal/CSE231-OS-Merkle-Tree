# CSE231-OS-Merkle-Tree
Got familiar with the Linux file system APIs by implementing a secure library (SecureFS) on top
of the existing file system interfaces that raises the alarm if the integrity of the
files created through the secure library APIs is compromised. At the high-level,
SecureFS maintains a Merkle tree for every file to check the consistency of file
blocks before every read and write. The root of the Merkle tree is saved on disk
to verify the integrity of files after reboot.
