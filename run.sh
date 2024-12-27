g++ "$1" -o program $(pkg-config --cflags --libs jsoncpp) -lcurl && ./program
