#include "dyn_array.hpp"
#include <vector>
#include <string>
#include <algorithm>

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
     * array_ptr in both versions
     * operator[] with multiple elements in both versions
     * at_unchecked in both versions
     * first and last in both versions
     * shrink_to_size
     * reset_single, reset_multiple, reset_all, reset_array
     * operator<<
    */

    // hdsa::DynArray<std::string> dyn {};
    // dyn.emplace_back("Test");
    // hdsa::DynArray<std::string> dyn {};
    // // dyn[0] = "This is ";
    // // dyn[1] = " the ";
    // // dyn[2] = " message.";

    // // dyn.pop_back();
    // // dyn.pop_back();
    // std::cout << "The size of dyn is: " << sizeof(dyn) << '\n';

    // hdsa::DynArray<std::string> dyn2 { std::move(dyn) };

    // // dyn.shrink_to_size();
    // // dyn.reset_array();
    // std::cout << dyn << '\n';

    // if (dyn.size() == 0) { std::cout << "dyn's size is 0\n"; }
    // else { std::cout << "dyn's size is bigger than 0: " << dyn.size() << '\n'; }

    // if (dyn.capacity() == 0) { std::cout << "dyn's capacity is 0\n"; }
    // else { std::cout << "dyn's capacity is bigger than 0: " << dyn.capacity() << '\n'; }

    // if (dyn.array_ptr() == 0) { std::cout << "dyn's buffer is null\n\n"; }
    // else { std::cout << "dyn's buffer has memory: " << dyn.array_ptr() << "\n\n"; }




    // if (dyn2.size() == 0) { std::cout << "dyn2's size is 0\n"; }
    // else { std::cout << "dyn2's size is bigger than 0: " << dyn2.size() << '\n'; }

    // if (dyn2.capacity() == 0) { std::cout << "dyn2's capacity is 0\n"; }
    // else { std::cout << "dyn2's capacity is bigger than 0: " << dyn2.capacity() << '\n'; }

    // if (dyn2.array_ptr() == 0) { std::cout << "dyn2's buffer is null\n\n"; }
    // else { std::cout << "dyn2's buffer has memory: " << dyn2.array_ptr() << "\n\n"; }

    // std::vector<std::string> vec(5ull);
    // vec[0] = "This is ";
    // vec[1] = " the ";
    // vec[2] = " message.";

    // if (vec.size() == 0) { std::cout << "vec's size is 0\n"; }
    // else { std::cout << "vec's size is bigger than 0: " << vec.size() << '\n'; }

    // if (vec.capacity() == 0) { std::cout << "vec's capacity is 0\n"; }
    // else { std::cout << "vec's capacity is bigger than 0: " << vec.capacity() << '\n'; }

    // if (vec.data() == 0) { std::cout << "vec's buffer is null\n\n"; }
    // else { std::cout << "vec's buffer has memory: " << vec.data() << "\n\n"; }


    hdsa::DynArray<Vec3> c { Vec3(4, 5, 6, 10), Vec3(5, 1, 6, 4), Vec3(9, 5, 7, 3), Vec3(7, 1, 4, 6) };

    /*
     * Loops for non-const iterator
     *
    */
    std::cout << "C-style loop\n";
    std::cout << "c's size is: " << c.size() << '\n';
    for (std::size_t i {}; i < c.size(); i++)
    {
        std::cout << c[i] << '\n';
    }

    std::cout << "C++ 98 style loop\n";
    for (auto it { c.begin() }; it != c.end(); ++it)
    {
        std::cout << *it << '\n';
    }

    std::cout << "std::algorithm loop\n";
    std::for_each(c.begin(), c.end(), [](auto &e)
    {
        std::cout << e << '\n';
    });

    std::cout << "Ranged-for loop\n";
    for(const auto& e: c)
    {
        std::cout << e << "\n\n\n";
    }



    /*
     * Loops for const iterator
     *
    */
    std::cout << "C-style loop\n";
    std::cout << "c's size is: " << c.size() << '\n';
    for (std::size_t i {}; i < c.size(); i++)
    {
        std::cout << c[i] << '\n';
    }

    std::cout << "C++ 98 style loop\n";
    for (auto it { c.cbegin() }; it != c.cend(); ++it)
    {
        std::cout << *it << '\n';
    }

    std::cout << "std::algorithm loop\n";
    std::for_each(c.cbegin(), c.cend(), [](auto &e)
    {
        std::cout << e << '\n';
    });

    std::cout << "Ranged-for loop\n";
    for(const auto& e: c)
    {
        std::cout << e << "\n\n\n";
    }



    /*
     * Loops for non-const reverse iterator
     *
    */
    std::cout << "C-style loop\n";
    std::cout << "c's size is: " << c.size() << '\n';
    for (std::size_t i { c.size() }; i > 0; --i)
    {
        std::cout << c[i - 1] << '\n';
    }

    std::cout << "C++ 98 style loop\n";
    for (auto it { c.rbegin() }; it != c.rend(); ++it)
    {
        std::cout << *it << '\n';
    }

    std::cout << "std::algorithm loop\n";
    std::for_each(c.rbegin(), c.rend(), [](auto &e)
    {
        std::cout << e << "\n\n\n";
    });



    /*
     * Loops for const reverse iterator
     *
    */
    std::cout << "C-style loop\n";
    std::cout << "c's size is: " << c.size() << '\n';
    for (std::size_t i { c.size() }; i > 0; --i)
    {
        std::cout << c[i - 1] << '\n';
    }

    std::cout << "C++ 98 style loop\n";
    for (auto it { c.crbegin() }; it != c.crend(); ++it)
    {
        std::cout << *it << '\n';
    }

    std::cout << "std::algorithm loop\n";
    std::for_each(c.crbegin(), c.crend(), [](auto &e)
    {
        std::cout << e << '\n';
    });

    return 0;
}
