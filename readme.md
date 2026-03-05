动态编译  依赖 msys2-dll
```
gcc colorpicker.c -o colorpicker.exe -mwindows -lkernel32 -luser32 -lgdi32 -Wl,--subsystem,windows
```


静态编译
```
gcc colorpicker.c -o colorpicker.exe -mwindows -static-libgcc -static-libstdc++ -lkernel32 -luser32 -lgdi32 -Wl,--subsystem,windows
```