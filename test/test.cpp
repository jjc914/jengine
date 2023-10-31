#include <assert.h>
#include <typeindex>

#include "../src/ecs/ecs.hpp"

//struct position {
//    float x, y, z;
//};
//
//struct rotation {
//    float x, y, z;
//};
//
//void move(double dt, std::vector<iarchetype*>) {
//    
//}

//void entity_create_test() {
//    position p;
//    p.x = 1;
//    p.y = 2;
//    p.z = 3;
//    rotation r;
//    r.x = 1;
//    r.y = 2;
//    r.z = 3;
//
//    ecs_manager ecs = ecs_manager();
//    entity e = ecs.create_entity<position, rotation>(p, r);
//
//}
//
//template <typename ...Ts>
//struct a;
//
//void sorted_type_set_test() {
////    assert(std::type_index(typeid(sort<double, int>)) == std::type_index(typeid(sort<int, double>)));
////    std::cout << std::type_index(typeid(sort<short int, unsigned int, long long int, signed char, long double>)).name() << std::endl;
////    std::cout << std::type_index(typeid(sort<long long int, unsigned int, signed char, long double, short int>)).name() << std::endl;
//    static_assert(std::is_same_v<sort_pass<sort_pass<long, int, short, char>::type>::type, int>);
//}

int main(int argc, const char *argv[]) {
//    entity_create_test();
//    sorted_type_set_test();
}
