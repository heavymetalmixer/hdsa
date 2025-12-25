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

        friend bool operator==(const Vec3& a, const Vec3& b)
    {
        if ((a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.mem == b.mem)) { return true; }

        return false;
    }
};

void iterators_tests()
{
    hdsa::DynArray<Vec3> d { Vec3(4, 6, 8, 2), Vec3{}, Vec3(1, 9, 3, 7), Vec3(1, 5, 9, 4), Vec3(3, 6, 5, 2) };
    hdsa::DynArray<Vec3>::iterator begin { d.begin() };

    std::cout << "Dereference test on position 0: \n";
    std::cout << "*begin is: " << *begin << '\n';
    std::cout << "begin[0] is: " << begin[0] << "\n\n";

    auto copy { begin++ };

    std::cout << "x++ test: \n";
    std::cout << "copy is begin++: " << *copy << '\n';
    std::cout << "begin++ is: " << *begin << "\n\n";

    ++copy;
    ++begin;

    std::cout << "++x test: \n";
    std::cout << "++copy is: " << *copy << '\n';
    std::cout << "++begin is: " << *begin << "\n\n";

    copy--;
    begin--;

    std::cout << "x-- test: \n";
    std::cout << "copy-- is: " << *copy << '\n';
    std::cout << "begin-- is: " << *begin << "\n\n";

    // --copy;
    --begin;

    std::cout << "--x test: \n";
    // std::cout << "--copy is: " << *copy << '\n';
    std::cout << "--begin is: " << *begin << "\n\n";

    // ++begin;
    // ++begin;
    // ++begin;
    // ++begin;
    // ++begin;

    // std::cout << "x++ out of upper bounds test: \n";
    // std::cout << "++begin is: " << *begin << "\n\n";

    begin +=1;

    std::cout << "x+=1 test: \n";
    std::cout << "begin+=1 is: " << *begin << "\n\n";

    begin +=0;

    std::cout << "x+=0 test: \n";
    std::cout << "begin+=0 is: " << *begin << "\n\n";

    begin -=1;

    std::cout << "x-=1 test: \n";
    std::cout << "begin-=1 is: " << *begin << "\n\n";

    begin -=0;

    std::cout << "x-=0 test: \n";
    std::cout << "begin-=0 is: " << *begin << "\n\n";

    begin +=4;

    std::cout << "x+=4 test: \n";
    std::cout << "begin+=4 is: " << *begin << "\n\n";

    begin -=4;

    std::cout << "x-=4 test: \n";
    std::cout << "begin-=4 is: " << *begin << "\n\n";

    begin = begin + 1;

    std::cout << "x = x + 1 test: \n";
    std::cout << "begin = begin + 1 is: " << *begin << "\n\n";

    begin = begin + 0;

    std::cout << "x = x + 0 test: \n";
    std::cout << "begin = begin + 0 is: " << *begin << "\n\n";

    begin = begin - 1;

    std::cout << "x = x - 1 test: \n";
    std::cout << "begin = begin - 1 is: " << *begin << "\n\n";

    begin = begin - 0;

    std::cout << "x = x - 0 test: \n";
    std::cout << "begin = begin - 0 is: " << *begin << "\n\n";

    begin = begin + 4;

    std::cout << "x = x + 4 test: \n";
    std::cout << "begin = begin + 4 is: " << *begin << "\n\n";

    begin = begin - 4;

    std::cout << "x = x - 4 test: \n";
    std::cout << "begin = begin - 4 is: " << *begin << "\n\n";

    ++begin;
    std::ptrdiff_t dif { begin - copy };

    std::cout << "std::ptrdiff_t x = begin - copy test: \n";
    std::cout << "dif is: " << dif << "\n\n";

    --begin;

    std::cout << "x + 4 test: \n";
    std::cout << "begin + 4 is: " << *(begin + 4) << "\n\n";

    std::cout << "x test: \n";
    std::cout << "begin now is: " << *begin << "\n\n";

    std::cout << "x == y test: \n";
    std::cout << "begin == copy is: " << (begin == copy) << "\n\n";

    std::cout << "x != y test: \n";
    std::cout << "begin != copy is: " << (begin != copy) << "\n\n";

    ++begin;

    std::cout << "x > y test: \n";
    std::cout << "begin > copy is: " << (begin > copy) << "\n\n";

    std::cout << "x < y test: \n";
    std::cout << "begin < copy is: " << (begin < copy) << "\n\n";

    ++copy;

    std::cout << "x >= y test: \n";
    std::cout << "begin >= copy is: " << (begin >= copy) << "\n\n";

    std::cout << "x <= y test: \n";
    std::cout << "begin <= copy is: " << (begin <= copy) << "\n\n";
}

void const_iterators_tests()
{
    hdsa::DynArray<Vec3> d { Vec3(4, 6, 8, 2), Vec3{}, Vec3(1, 9, 3, 7), Vec3(1, 5, 9, 4), Vec3(3, 6, 5, 2) };
    hdsa::DynArray<Vec3>::const_iterator begin { d.cbegin() };

    std::cout << "Dereference test on position 0: \n";
    std::cout << "*begin is: " << *begin << '\n';
    std::cout << "begin[0] is: " << begin[0] << "\n\n";

    auto copy { begin++ };

    std::cout << "x++ test: \n";
    std::cout << "copy is begin++: " << *copy << '\n';
    std::cout << "begin++ is: " << *begin << "\n\n";

    ++copy;
    ++begin;

    std::cout << "++x test: \n";
    std::cout << "++copy is: " << *copy << '\n';
    std::cout << "++begin is: " << *begin << "\n\n";

    copy--;
    begin--;

    std::cout << "x-- test: \n";
    std::cout << "copy-- is: " << *copy << '\n';
    std::cout << "begin-- is: " << *begin << "\n\n";

    // --copy;
    --begin;

    std::cout << "--x test: \n";
    // std::cout << "--copy is: " << *copy << '\n';
    std::cout << "--begin is: " << *begin << "\n\n";

    // ++begin;
    // ++begin;
    // ++begin;
    // ++begin;
    // ++begin;

    // std::cout << "x++ out of upper bounds test: \n";
    // std::cout << "++begin is: " << *begin << "\n\n";

    begin +=1;

    std::cout << "x+=1 test: \n";
    std::cout << "begin+=1 is: " << *begin << "\n\n";

    begin +=0;

    std::cout << "x+=0 test: \n";
    std::cout << "begin+=0 is: " << *begin << "\n\n";

    begin -=1;

    std::cout << "x-=1 test: \n";
    std::cout << "begin-=1 is: " << *begin << "\n\n";

    begin -=0;

    std::cout << "x-=0 test: \n";
    std::cout << "begin-=0 is: " << *begin << "\n\n";

    begin +=4;

    std::cout << "x+=4 test: \n";
    std::cout << "begin+=4 is: " << *begin << "\n\n";

    begin -=4;

    std::cout << "x-=4 test: \n";
    std::cout << "begin-=4 is: " << *begin << "\n\n";

    begin = begin + 1;

    std::cout << "x = x + 1 test: \n";
    std::cout << "begin = begin + 1 is: " << *begin << "\n\n";

    begin = begin + 0;

    std::cout << "x = x + 0 test: \n";
    std::cout << "begin = begin + 0 is: " << *begin << "\n\n";

    begin = begin - 1;

    std::cout << "x = x - 1 test: \n";
    std::cout << "begin = begin - 1 is: " << *begin << "\n\n";

    begin = begin - 0;

    std::cout << "x = x - 0 test: \n";
    std::cout << "begin = begin - 0 is: " << *begin << "\n\n";

    begin = begin + 4;

    std::cout << "x = x + 4 test: \n";
    std::cout << "begin = begin + 4 is: " << *begin << "\n\n";

    begin = begin - 4;

    std::cout << "x = x - 4 test: \n";
    std::cout << "begin = begin - 4 is: " << *begin << "\n\n";

    ++begin;
    std::ptrdiff_t dif { begin - copy };

    std::cout << "std::ptrdiff_t x = begin - copy test: \n";
    std::cout << "dif is: " << dif << "\n\n";

    --begin;

    std::cout << "x + 4 test: \n";
    std::cout << "begin + 4 is: " << *(begin + 4) << "\n\n";

    std::cout << "x test: \n";
    std::cout << "begin now is: " << *begin << "\n\n";

    std::cout << "x == y test: \n";
    std::cout << "begin == copy is: " << (begin == copy) << "\n\n";

    std::cout << "x != y test: \n";
    std::cout << "begin != copy is: " << (begin != copy) << "\n\n";

    ++begin;

    std::cout << "x > y test: \n";
    std::cout << "begin > copy is: " << (begin > copy) << "\n\n";

    std::cout << "x < y test: \n";
    std::cout << "begin < copy is: " << (begin < copy) << "\n\n";

    ++copy;

    std::cout << "x >= y test: \n";
    std::cout << "begin >= copy is: " << (begin >= copy) << "\n\n";

    std::cout << "x <= y test: \n";
    std::cout << "begin <= copy is: " << (begin <= copy) << "\n\n";
}

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

    // const_iterators_tests();

    hdsa::DynArray<Vec3> v1 { Vec3(6, 4, 5, 2) };
    hdsa::DynArray<Vec3> v2 { v1 };

    std::cout << (v1 = v1) << '\n';

    return 0;
}
