g++ -Wall --std=c++20 -O3 -DPERCEPTRON -DHHT_ENABLE -o prog/both1 src/hawkeye_algorithm.cc lib/config1.a
g++ -Wall --std=c++20 -O3 -DPERCEPTRON -o prog/perceptron1 src/hawkeye_algorithm.cc lib/config1.a
g++ -Wall --std=c++20 -O3 -DHHT_ENABLE -o prog/ehc1 src/hawkeye_algorithm.cc lib/config1.a
g++ -Wall --std=c++20 -O3 -o prog/hawkeye1 src/hawkeye_algorithm.cc lib/config1.a
g++ -Wall --std=c++20 -O3 -o prog/lru1 src/lru.cc lib/config1.a

g++ -Wall --std=c++20 -O3 -DPERCEPTRON -DHHT_ENABLE -o prog/both2 src/hawkeye_algorithm.cc lib/config2.a
g++ -Wall --std=c++20 -O3 -DPERCEPTRON -o prog/perceptron2 src/hawkeye_algorithm.cc lib/config2.a
g++ -Wall --std=c++20 -O3 -DHHT_ENABLE -o prog/ehc2 src/hawkeye_algorithm.cc lib/config2.a
g++ -Wall --std=c++20 -O3 -o prog/hawkeye2 src/hawkeye_algorithm.cc lib/config2.a
g++ -Wall --std=c++20 -O3 -o prog/lru2 src/lru.cc lib/config2.a
