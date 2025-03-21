printf "Compiling...\n"
gcc -Wall src/glx.c src/gl3w.c -o glx -I src/ -lX11 -lGL -lm -lXfixes
if [ $? -eq 0 ]; then
    printf "Compilation was \033[0;32m\033[1msuccessful\033[0m.\n"
fi
