#include <vector>
#include <string>
#include "dyn_array.hpp"
#include "pmr.hpp"
#include <type_traits>

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

    explicit Vec3(const allocator_type& allocator) noexcept
    : x { 2 },
      y { 3 },
      z { 4 },
      alloc { allocator },
      mem { alloc.new_object<int>(10) }
    {
        std::cout << "Vec3 Default Construction with allocator assignment.\n";
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

    explicit Vec3(int xa, int ya, short za, int mema, const allocator_type& allocator)
    : x { xa },
      y { ya },
      z { za },
      alloc { allocator },
    //   mem { alloc.new_object<int>(mema) }
      mem { alloc.new_object<int>(mema) }
    {
        std::cout << "mem: " << mem << '\n';
        std::cout << "Vec3 Value Construction with allocator assignment.\n";
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

    Vec3(const Vec3& other, const allocator_type& allocator)
    : x { other.x },
      y { other.y },
      z { other.z },
      alloc { allocator },
      mem { alloc.new_object<int>(*other.mem) }
    {
        std::cout << "Vec3 Copy Construction with allocator assignment.\n";
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

struct UnalignedTest
{
    char c1 { 'c' };
};

struct temp
{
    int32_t a { 3 };
    int16_t b { 5 };
};

int main()
{
    // void* buffer1 { ::operator new(((sizeof(uint8_t) * sizeof(Vec3)) + alignof(int)) * 4, static_cast<std::align_val_t>(alignof(uint8_t)), std::nothrow) };
    // hdsa::ArenaAlloc memory1 { buffer1, (sizeof(uint8_t) * sizeof(Vec3)) + alignof(int) };
    // hdsa::ArenaAlloc memory2 {};
    // memory2 = memory1;


    // Vec3* v1 { static_cast<Vec3*>(memory1.allocate((sizeof(uint8_t) * sizeof(Vec3)), alignof(Vec3))) };
    // new(v1) Vec3(3, 3, 3, 4, std::pmr::polymorphic_allocator<uint8_t>(&memory1));
    // std::cout << "v1 is: " << *v1 << '\n';
    // std::cout << "mem is: " << v1->mem << '\n';

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    std::cout << "Size of SYSTEM_INFO: " << sizeof(SYSTEM_INFO) << '\n';
    std::cout << "Page size: " << static_cast<size_t>(sys_info.dwPageSize) << '\n';
    std::cout << "Granularity size: " << static_cast<size_t>(sys_info.dwAllocationGranularity) << '\n';
    std::cout << "Round up test: " << hdsa::round_to_block(65535, static_cast<size_t>(sys_info.dwAllocationGranularity)) << "\n\n\n";

    // uint8_t* memory { static_cast<uint8_t*>(VirtualAlloc(nullptr, 1073741824, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)) };
    // uint8_t* memory2 { memory + 65536 };
    // memory2 = static_cast<uint8_t*>(VirtualAlloc(memory2, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));

    // void* temp { static_cast<void*>(memory) };
    // int32_t* a { static_cast<int32_t*>(temp) };
    // temp = static_cast<void*>(static_cast<uint8_t*>(temp) + sizeof(int32_t));
    // int32_t* b { static_cast<int32_t*>(temp) };

    // for (size_t i {}; i < 1073741824; ++i)
    // {
    //     memory[i] = 0;
    // }

    // memset(memory, 0, 1073741824);
    // std::cout << "Finished allocating 1GB!\n";

    hdsa::DynArray<int32_t> d;
    d = {};
    std::cout << (d.is_empty() ? "True" : "False") << '\n';

    hdsa::VirtualPageAlloc pa {};
    pa.page_allocate(458753);
    std::cout << "pa buffer_length: " << pa.buffer_length << '\n';
    std::cout << "pa initial address: " << reinterpret_cast<uintptr_t>(pa.buffer) << '\n';
    std::cout << "pa final address: " << reinterpret_cast<uintptr_t>(pa.end_address) << '\n';
    std::cout << "pa after end_address: " << (reinterpret_cast<uintptr_t>(pa.end_address) + 1) << "\n\n\n";

    hdsa::StackAlloc vp { pa.buffer, pa.buffer_length };
    void* temp_buffer { vp.stack_allocate((32 * sizeof(uint64_t))) };

    hdsa::PoolAlloc pool {};
    pool.assign_buffer(temp_buffer, (32 * sizeof(uint64_t)), sizeof(uint64_t), alignof(uint8_t));
    std::cout << "Is temp_buffer aligned? " << ((hdsa::is_aligned(static_cast<void*>(temp_buffer), alignof(uint64_t))) ? "Yes!" :  "No!") << '\n';
    std::cout << "temp_buffer: " << reinterpret_cast<uintptr_t>(temp_buffer) << '\n';
    std::cout << "temp_buffer + sizeof(StackHeader): " << (reinterpret_cast<uintptr_t>(temp_buffer) + static_cast<uintptr_t>(sizeof(hdsa::StackHeader))) << '\n';

    temp* x { static_cast<temp*>(pool.pool_allocate()) };
    new(x) temp();
    std::cout << "x: " << static_cast<void*>(x) << '\n';
    std::cout << "Is x aligned? " << ((hdsa::is_aligned(static_cast<void*>(x), alignof(temp))) ? "Yes!" :  "No!") << '\n';
    std::cout << "x->a: " << x->a << '\n';
    std::cout << "x->b: " << x->b << "\n\n\n";

    uint8_t* uc { static_cast<uint8_t*>(pool.pool_allocate()) };
    std::cout << "Is uc aligned? " << ((hdsa::is_aligned(static_cast<void*>(uc), 8)) ? "Yes!" :  "No!") << '\n';
    uc[0] = 'x';
    uc[1] = 'y';
    uc[2] = 'z';
    uc[3] = '1';
    uc[4] = '2';
    std::cout << "uc: " << static_cast<void*>(uc) << '\n';
    std::cout << "uc[0]: " << uc[0] << '\n';
    std::cout << "uc[1]: " << uc[1] << '\n';
    std::cout << "uc[2]: " << uc[2] << '\n';
    std::cout << "uc[3]: " << uc[3] << '\n';
    std::cout << "uc[4]: " << uc[4] << "\n\n\n";

    uint32_t* star { static_cast<uint32_t*>(pool.pool_allocate()) };
    std::cout << "star: " << static_cast<void*>(star) << '\n';
    std::cout << "Is star aligned? " << ((hdsa::is_aligned(static_cast<void*>(star), alignof(uint32_t))) ? "Yes!" :  "No!") << '\n';
    *star = 10;
    std::cout << "*star: " << *star << "\n\n\n";

    uint8_t* uc2 { static_cast<uint8_t*>(pool.pool_allocate()) };
    std::cout << "Is uc2 aligned? " << ((hdsa::is_aligned(static_cast<void*>(uc2), 8)) ? "Yes!" :  "No!") << '\n';
    uc2[0] = 'a';
    uc2[1] = 'b';
    uc2[2] = 'c';
    uc2[3] = 'd';
    uc2[4] = 'e';
    std::cout << "uc2: " << static_cast<void*>(uc2) << '\n';
    std::cout << "uc2[0]: " << uc2[0] << '\n';
    std::cout << "uc2[1]: " << uc2[1] << '\n';
    std::cout << "uc2[2]: " << uc2[2] << '\n';
    std::cout << "uc2[3]: " << uc2[3] << '\n';
    std::cout << "uc2[4]: " << uc2[4] << "\n\n\n";

    uint16_t* us { static_cast<uint16_t*>(pool.pool_allocate()) };
    std::cout << "Is us aligned? " << ((hdsa::is_aligned(static_cast<void*>(us), alignof(uint16_t))) ? "Yes!" :  "No!") << '\n';
    us[0] = 12;
    us[1] = 14;
    us[2] = 16;
    std::cout << "us: " << static_cast<void*>(us) << '\n';
    std::cout << "us[0]: " << us[0] << '\n';
    std::cout << "us[1]: " << us[1] << '\n';
    std::cout << "us[2]: " << us[2] << "\n\n\n";

    pool.pool_deallocate(static_cast<void*>(x));
    pool.pool_deallocate(uc);
    pool.pool_deallocate(star);
    pool.pool_deallocate(uc2);
    pool.pool_deallocate(us);

    // pool.stack_pop();
    // pool.stack_pop();
    // pool.stack_pop();
    // pool.stack_pop();
    // pool.stack_pop();
    // arena1.stack_pop();

    vp.stack_deallocate();
    pa.page_deallocate();

    std::cin.get();

    // *a = 5;
    // *b = 3;

    // std::cout << "memory: " << (static_cast<void*>(memory)) << '\n';
    // std::cout << "memory2: " << (static_cast<void*>(memory2)) << '\n';
    // std::cout << "a: " << (static_cast<void*>(a)) << '\n';
    // std::cout << "b: " << (static_cast<void*>(b)) << '\n';
    // std::cout << "*a: " << *a << '\n';
    // std::cout << "*b: " << *b << '\n';

    // VirtualFree(memory2, 0, MEM_DECOMMIT);
    // VirtualFree(memory, 0, MEM_DECOMMIT);
    // VirtualFree(memory2, 0, MEM_RELEASE);
    // VirtualFree(memory, 0, MEM_RELEASE);

    return 0;
}
