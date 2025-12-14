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
     * Copy constructor
     * std::initializer_list constructor
     * Move constructor
     * Copy assignment
     * std::initializer_list assignment
     * Move assigment
     * reserve
     * push_back in both versions
     * emplace_back
     * pop_back
     * operator[] with multiple elements
     * operator<<
    */

    hdsa::DynArray<std::string> dyn0 { "Test" };

    hdsa::DynArray<std::string> dyn {};
    dyn.emplace_back("Test");
    // hdsa::DynArray<std::string> dyn(3ull, "");
    // dyn[0] = "This is ";
    // dyn[1] = " the ";
    // dyn[2] = " message.";
    // dyn.pop_back();
    // dyn.pop_back();
    // dyn.pop_back();

    hdsa::DynArray<std::string> dyn2 {};
    // hdsa::DynArray<std::string> dyn2 { dyn };
    // hdsa::DynArray<std::string> dyn2 { std::move(dyn) };
    dyn2 = std::move(dyn);

    // std::cout << dyn2 << '\n';

    if (dyn.size() == 0) { std::cout << "dyn's size is 0\n"; }
    else { std::cout << "dyn's size is bigger than 0: " << dyn.size() << '\n'; }

    if (dyn.capacity() == 0) { std::cout << "dyn's capacity is 0\n"; }
    else { std::cout << "dyn's capacity is bigger than 0: " << dyn.capacity() << '\n'; }

    if (dyn.array_ptr() == 0) { std::cout << "dyn's buffer is null\n\n"; }
    else { std::cout << "dyn's buffer has memory: " << dyn.array_ptr() << "\n\n"; }



    if (dyn2.size() == 0) { std::cout << "dyn2's size is 0\n"; }
    else { std::cout << "dyn2's size is bigger than 0: " << dyn2.size() << '\n'; }

    if (dyn2.capacity() == 0) { std::cout << "dyn2's capacity is 0\n"; }
    else { std::cout << "dyn2's capacity is bigger than 0: " << dyn2.capacity() << '\n'; }

    if (dyn2.array_ptr() == 0) { std::cout << "dyn2's buffer is null\n\n"; }
    else { std::cout << "dyn2's buffer has memory: " << dyn2.array_ptr() << "\n\n"; }

    return 0;
}
