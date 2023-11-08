# Visual EDitor

Simple visual text editor written in pure C.

There's nothing here, just a playground. It's not usefull because of:  
- Inserting/reading unicode causes segfault;  
- Writing/reading a line bigger than the window width also causes segfault;  
- Tab character is literally unsupported;  
- The file name should not be bigger than 128 bytes.

## Quickstart

`$ make`  
`$ ./ved akatsuki.txt`

## Keybinds

- Move left: `Ctrl + B` or `Left`.  
- Move right: `Ctrl + F` or `Right`.  
- Move up: `Ctrl + P` or `Up`.  
- Move down: `Ctrl + N` or `Down`.  
- Move to end of line: `Ctrl + E` or `End`.  
- Move to beggining of line: `Ctrl + A` or `Home`.  
- Delete character under cursor: `Ctrl + D` or `Del`.  
- Exit: `Ctrl + x`.
