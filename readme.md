## msys2mingw64

install
```
pacman -S mingw-w64-x86_64-gcc make
```

build
```
make clean
make all
```

## linux

install
```
sudo apt install libx11-dev libxrandr-dev
```

build
```
gcc colorpicker.linux.c -o colorpicker -lX11 -lXext -lXrandr -lXfixes
```
