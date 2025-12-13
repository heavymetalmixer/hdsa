#include "dyn_array.hpp"
#include <vector>
#include <string>

struct Vec3
{
    int x { 1 };
    int y { 2 };
    int z { 3 };
    int* mem { nullptr };

    Vec3()
    : mem { new int(10) }
    {}

    Vec3(int xa, int ya, int za, int mema)
    : x { xa },
      y { ya },
      z { za },
      mem { new int(mema) }
    {}

    Vec3(const Vec3& other)
    : x { other.x },
      y { other.y },
      z { other.z },
      mem { new int(*other.mem) }
    {
        std::cout << "Vec3 Copy Construction.\n";
    }

    Vec3(Vec3&& other) noexcept
    : x { other.x },
      y { other.y },
      z { other.z },
      mem { other.mem }
    {
        other.x = 1;
        other.y = 2;
        other.z = 3;
        other.mem = nullptr;

        std::cout << "Vec3 Move Construction.\n";
    }

    Vec3& operator=(const Vec3& other) = default;
    Vec3& operator=(Vec3&& other) noexcept = default;

    ~Vec3()
    {
        delete mem;
        mem = nullptr;
    }

    friend std::ostream& operator <<(std::ostream& out, const Vec3& v3)
    {
        out << "Vec3 { " << v3.x << ", " << v3.y << ", " << v3.z << ", " << *v3.mem << " }";
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
     * Copy constructor PARTIALLY
     * std::initializer_list constructor
     * Move constructor PARTIALLY
     * Copy assignment PARTIALLY
     * std::initializer_list assignment PARTIALLY
     * Move assigment PARTIALLY
     * push_back in both versions
     * emplace_back
     * pop_back
     * operator[] with multiple elements
     * operator<<
    */

    hdsa::DynArray<std::string> dyn0 {};

    if (dyn0.array_ptr() == nullptr)
    {
        std::cout << "dyn0's m_first_ptr is NULL\n";
    }

    hdsa::DynArray<std::string> dyn(3ull);
    dyn[0] = "This is ";
    std::cout << "dyn Size: " << dyn.size() << '\n';
    dyn[1] = " the ";
    dyn[2] = " message.";
    dyn.pop_back();
    dyn.pop_back();
    dyn.pop_back();

    hdsa::DynArray<std::string> dyn2 { dyn };
    // hdsa::DynArray<std::string> dyn2 { std::move(dyn) };
    // dyn2 = dyn;

    //std::cout << dyn2 << '\n';

    int a { 10 };
    hdsa::DynArray<Vec3> v31 { Vec3(4, 5, 6, a) } ;

    hdsa::DynArray<Vec3> v32{};
    v32.emplace_back();
    v32.emplace_back();
    std::cout << v32 << '\n';

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
