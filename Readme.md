# Linux only, no memwatch support on Windows
## Netplay works with these additions
### Install Directions:
1. Navigate to the directory where you want the dolphin folder to be in
2. `git clone https://github.com/dolphin-emu/dolphin`
3. `cd dolphin`
4. `git checkout ce493b897d6d3735c930a8465cc0c26bbe5feb86`
5. `wget https://raw.githubusercontent.com/XeSquirmy/dolphin-7840-memorywatcher/master/diff.patch`
6. `patch -p1 < diff.patch`
7. `mkdir Build && cd Build`
8. `cmake ..`
9. `make`
10. If you want to install, run `make install`
