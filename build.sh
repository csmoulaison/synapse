printf "Compiling...\n"
gcc -Wall src/x11_synapse.c src/gl3w.c -o synapse -O3 -I src/ -lX11 -lGL -lm -lXfixes
if [ $? -eq 0 ]; then
    printf "Compilation was \033[0;32m\033[1msuccessful\033[0m.\n"
fi
