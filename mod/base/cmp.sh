g++ -w -fvisibility=hidden -shared -fPIC -Wl,-z,relro,-z,now -Ofast -std=gnu++17 -I ../base -I ../../include base.cpp -o ../../out/base.so
