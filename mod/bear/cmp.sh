g++ -fvisibility=hidden -shared -fPIC -Wl,-z,relro,-z,now -Ofast -std=gnu++17 -I ../base/ -I ../../include bear.cpp -o ../../out/bear.so

