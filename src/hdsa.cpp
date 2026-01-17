#include <vector>
#include <string>
#include <algorithm>
#include <print>
#include "dyn_array.hpp"
#include "pmr.hpp"

struct Vec3
{
    using allocator_type = std::pmr::polymorphic_allocator<uint8_t>;

    int x {};
    int y {};
    short z {};
    allocator_type alloc { std::pmr::new_delete_resource() };
    int* mem { nullptr };


    Vec3()
    : alloc { allocator_type(std::pmr::new_delete_resource()) },
      mem { alloc.new_object<int>(10) }
    {
        std::cout << "Vec3 Value Construction.\n";
    }

    explicit Vec3(int xa, int ya, short za, int mema)
    : x { xa },
      y { ya },
      z { za },
      alloc { allocator_type(std::pmr::new_delete_resource()) },
      mem { alloc.new_object<int>(mema) }
    {
        std::cout << "Vec3 Value Construction.\n";
    }

    Vec3(const Vec3& other)
    : x { other.x },
      y { other.y },
      z { other.z },
      alloc { allocator_type(std::pmr::new_delete_resource()) },
      mem { alloc.new_object<int>(*other.mem) }
    {
        std::cout << "Vec3 Copy Construction.\n";
    }

    Vec3(Vec3&& other) noexcept
    : x { other.x },
      y { other.y },
      z { other.z },
      alloc { allocator_type(std::pmr::new_delete_resource()) },
      mem { alloc.new_object<int>(std::move(*other.mem)) }
    {
        std::cout << "Vec3 Move Construction.\n";
    }

    explicit Vec3(const allocator_type& allocator) noexcept
    : x { 2 },
      y { 3 },
      z { 4 },
      alloc { allocator },
      mem { alloc.new_object<int>(10) }
    {
        std::cout << "Vec3 Default Construction with allocator assignment.\n";
    }

    explicit Vec3(int xa, int ya, short za, int mema, const allocator_type& allocator)
    : x { xa },
      y { ya },
      z { za },
      alloc { allocator },
      mem { alloc.new_object<int>(mema) }
    {
        std::cout << "mem: " << mem << '\n';
        std::cout << "Vec3 Value Construction with allocator assignment.\n";
    }

    Vec3(const Vec3& other, const allocator_type& allocator)
    : x { other.x },
      y { other.y },
      z { other.z },
      alloc { allocator },
      mem { alloc.new_object<int>(*other.mem) }
    {
        std::cout << "Vec3 Copy Construction with allocator assignment.\n";
    }

    Vec3(Vec3&& other, const allocator_type& allocator)
    : x { other.x },
      y { other.y },
      z { other.z },
      alloc { allocator },
      mem { alloc.new_object<int>(std::move(*other.mem)) }
    {
        std::cout << "Vec3 Move Construction with allocator assignment.\n";
    }

    allocator_type get_allocator() const
    {
        return alloc;
    }

    ~Vec3()
    {
        // delete mem;
        if (mem != nullptr) { alloc.delete_object(mem); }
        mem = nullptr;

        std::cout << "Vec3 Destruction.\n";
    }

    Vec3& operator=(const Vec3& other)
    {
        x = other.x;
        y = other.y;
        z = other.z;
        *mem = *other.mem;

        std::cout << "Vec3 Copy Assignment.\n";

        return *this;
    }

    Vec3& operator=(Vec3&& other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        *mem = std::move(*other.mem);

        std::cout << "Vec3 Move Assignment.\n";

        return *this;
    }

    friend std::ostream& operator <<(std::ostream& out, const Vec3& v)
    {
        out << "Vec3 { " << v.x << ", " << v.y << ", " << v.z << ", " << ((v.mem == nullptr) ? 0 : *v.mem) << " }";
        return out;
    }

    friend bool operator==(const Vec3& a, const Vec3& b)
    {
        if ((a.x == b.x) && (a.y == b.y) && (a.z == b.z) && (a.alloc == b.alloc) && (*a.mem == *b.mem)) { return true; }

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
    // std::pmr::monotonic_buffer_resource memory1 { sizeof(int) };
    // std::pmr::polymorphic_allocator<unsigned char> poly1 { &memory1 };

    // Vec3 v1 { poly1 };
    // std::cout << "v1 is: " << v1 << '\n';


    // std::pmr::monotonic_buffer_resource memory2 { sizeof(int) };
    // std::pmr::polymorphic_allocator<unsigned char> poly2 { &memory2 };

    // Vec3 v2 { 3, 3, 3, 4, poly2 };
    // std::cout << "v2 is: " << v2 << '\n';


    // v2 = std::move(v1);
    // std::cout << "v2 is: " << v2 << '\n';
    // std::cout << "------------------------------------------------------\n\n";
    // std::cout << "v1 is: " << v1 << '\n';

    uint8_t* buffer1 { static_cast<uint8_t*>(::operator new(((sizeof(uint8_t) * sizeof(Vec3)) + alignof(int)) * 4, static_cast<std::align_val_t>(alignof(uint8_t)), std::nothrow)) };
    hdsa::ArenaAlloc memory1 { static_cast<void*>(buffer1), (sizeof(uint8_t) * sizeof(Vec3)) + alignof(int) };


    Vec3* v1 { static_cast<Vec3*>(memory1.allocate((sizeof(uint8_t) * sizeof(Vec3)), alignof(Vec3))) };
    new(v1) Vec3(3, 3, 3, 4, std::pmr::polymorphic_allocator<uint8_t>(&memory1));
    std::cout << "v1 is: " << *v1 << '\n';
    std::cout << "mem is: " << v1->mem << '\n';

    // v1->mem = new(v1->mem) int(10);
    // std::cout << "v1 is: " << *v1 << '\n';


    // uint8_t* buffer2 { static_cast<uint8_t*>(::operator new((sizeof(uint8_t) * sizeof(Vec3)) + sizeof(int), static_cast<std::align_val_t>(alignof(uint8_t)), std::nothrow)) };
    // hdsa::ArenaAlloc memory2 { static_cast<void*>(buffer2), sizeof(Vec3) };
    // std::pmr::polymorphic_allocator<unsigned char> poly2 { &memory2 };

    // Vec3 v2 { 3, 3, 3, 4, poly2 };
    // std::cout << "v2 is: " << v2 << '\n';


    // v2 = v1;
    // std::cout << "v2 is: " << v2 << '\n';
    // std::cout << "------------------------------------------------------\n\n";
    // std::cout << "v1 is: " << v1 << '\n';

    // constexpr std::size_t buffer_size { sizeof(Vec3) };
    // uint8_t buffer1[buffer_size];
    // Vec3* v1 { new(&buffer1) Vec3(4, 2, 9, 8) };
    // std::cout << "v1: " << alignof(v1) << '\n';

    // uint8_t buffer2[buffer_size];
    // std::memcpy(&buffer2, &buffer1, buffer_size);
    // Vec3* v2 { reinterpret_cast<Vec3*>(&buffer2) };
    // std::cout << "v2: " << alignof(v2) << '\n';

    return 0;
}
