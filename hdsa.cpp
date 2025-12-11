#include "dyn_array.hpp"
#include <vector>
#include <string>

struct Vec3
{
    int x { 1 };
    int y { 2 };
    int z { 3 };
    int* mem = nullptr;

    Vec3()
    {
        mem = new int(10);
    }

    Vec3(const Vec3& other)
    : x { other.x },
      y { other.y },
      z { other.z }
    {
        mem = new int(*other.mem);
    }

    Vec3(Vec3&& other) noexcept = default;
    Vec3& operator=(const Vec3& other) = default;
    Vec3& operator=(Vec3&& other) noexcept = default;

    ~Vec3()
    {
        delete mem;
        mem = nullptr;
    }

    friend std::ostream& operator <<(std::ostream& out, const Vec3& v3)
    {
        out << "Vec3 { " << v3.x << ", " << v3.y << ", " << v3.z << ", " << *(v3.mem) << " }";
        return out;
    }
};

int main()
{
    /**
     * TESTED_ALREADY:
     * Can the private members be modified? NO
     * Default constructor
     * Constructor with single std::size_t argument
     * Constructor with std::size_t and element arguments
     * std::initializer_list constructor
     * Copy constructor PARTIALLY
     * Copy assignment PARTIALLY
     * std::initializer_list assignment PARTIALLY
     * push_back in both versions
     * operator[] with multiple elements
     * Move constructor PARTIALLY
    */

    hdsa::DynArray<std::size_t> dyn0 {};

    if (dyn0.array_ptr() == nullptr)
    {
        std::cout << "dyn0's m_first_ptr is NULL\n";
    }

    hdsa::DynArray<std::string> dyn(3ull);
    // dyn[0] = "This is ";
    // std::cout << sizeof(dyn[0]) << '\n';
    // dyn[1] = " the ";
    // dyn[2] = " message.";

    hdsa::DynArray<std::string> dyn2 { dyn };
    // hdsa::DynArray<std::string> dyn2 { std::move(dyn) };
    // dyn2 = dyn;

    // std::cout << dyn2 << '\n';


    // auto a { dyn2.cend() - 1 };
    // // a = dyn2.const_begin()[1];

    // std::cout << "The const iterator is at: " << a.data() << '\n';
    // // a++;
    // a = a - 2;
    // std::cout << "The const iterator now is: " << *a << '\n';
    // std::cout << a[0] << '\n';

    // hdsa::DynArray<std::string> dyn3(4ull, "Word");

    return 0;
}
