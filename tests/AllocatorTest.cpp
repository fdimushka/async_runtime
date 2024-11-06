#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_ENABLE_BENCHMARKING


#include "catch.hpp"
#include "ar/runtime.hpp"


using namespace AsyncRuntime;
using namespace std;

SCENARIO( "Object allocator test") {
    SetupRuntime();

    static int foo_state = 0;
    class Foo {
    public:
        Foo(): value(0) {
            foo_state = 1;
        };

        Foo(int v): value(v) {
            foo_state = 1;
        };

        ~Foo() {
            foo_state = 0;
        }

        int value;
    };

    GIVEN("make shared ptr") {
        {
            std::shared_ptr<Foo> foo_ptr = AsyncRuntime::make_shared_ptr<Foo>(GetResource(), 100);
            REQUIRE(foo_state == 1);
            REQUIRE(foo_ptr->value == 100);
        }
        REQUIRE(foo_state == 0);
    }


    GIVEN("make unique ptr") {
        {
            auto foo_ptr = AsyncRuntime::make_unique_ptr<Foo>(GetResource(), 200);
            REQUIRE(foo_state == 1);
            REQUIRE(foo_ptr->value == 200);
        }
        REQUIRE(foo_state == 0);
    }


    Terminate();
}

SCENARIO( "Inherited object allocator test") {
    SetupRuntime();

    static int foo_state = 0;
    class Base {
    public:
        Base(int v) : base_value(v) {};
        virtual ~Base() = default;

        int base_value;
    };

    class Foo : public Base {
    public:
        Foo(): value(0), Base(0) {
            foo_state = 1;
        };

        Foo(int v): value(v), Base(v+100) {
            foo_state = 1;
        };

        ~Foo() {
            foo_state = 0;
        }

        int value;
    };

    class Empty {
    public:
        Empty(){
            foo_state = 1;
        };
        ~Empty(){
            foo_state = 0;
        };
    };

    GIVEN("make shared ptr") {
        {
            std::shared_ptr<Base> foo_ptr = AsyncRuntime::make_shared_ptr<Foo>(GetResource(), 100);
            REQUIRE(foo_state == 1);
            REQUIRE(((Foo*)foo_ptr.get())->value == 100);
            REQUIRE(foo_ptr->base_value == 200);
        }
        REQUIRE(foo_state == 0);

        {
            std::shared_ptr<Empty> foo_ptr = AsyncRuntime::make_shared_ptr<Empty>(GetResource());
            REQUIRE(foo_state == 1);
        }
        REQUIRE(foo_state == 0);
    }


    GIVEN("make unique ptr") {
        {
            auto foo_ptr = AsyncRuntime::make_unique_ptr<Foo>(GetResource(), 200);
            REQUIRE(foo_state == 1);
            REQUIRE(((Foo*)foo_ptr.get())->value == 200);
            REQUIRE(foo_ptr->base_value == 300);
        }
        REQUIRE(foo_state == 0);
    }


    Terminate();
}


SCENARIO( "allocator construct_at/destroy_at") {
    static int foo_state = 0;
    class Base {
     public:
      Base(int v) : base_value(v) {};
      virtual ~Base() = default;

      int base_value;
    };

    class Foo : public Base {
     public:
      Foo(): value(0), Base(0) {
        foo_state = 1;
      };

      Foo(int v): value(v), Base(v+100) {
        foo_state = 1;
      };

      ~Foo() {
        foo_state = 0;
      }

      int value;
    };

    class Empty {
     public:
      Empty(){
        foo_state = 1;
      };
      ~Empty(){
        foo_state = 0;
      };
    };

    SetupRuntime();

    GIVEN("construct_at") {
        Allocator<Foo> allocator(GetResource());
        Foo *foo = allocator.construct_at(100);
        REQUIRE(foo_state == 1);
        REQUIRE(foo->value == 100);
        REQUIRE(foo->base_value == 200);

        Allocator<Foo> allocator_del(GetResource());
        allocator_del.destroy_at(foo);
        REQUIRE(foo_state == 0);
    }

    Terminate();
}


SCENARIO( "allocator for stl containers") {
    SetupRuntime();

    GIVEN("vector") {
        {
            Allocator<int> allocator(GetResource());
            std::vector<int, Allocator<int>> vec(100, allocator);
            for (int i = 0; i < 100; ++i) {
                vec[i] = i;
            }
        }

        {
            std::vector<int, Allocator<int>> vec;
            for (int i = 0; i < 100; ++i) {
                vec.push_back(i);
            }
        }
    }

    GIVEN("string") {
        using String = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
        String str;
        str = "hello allocator";
        REQUIRE(str == "hello allocator");

        String str2("hello preallocated");
        REQUIRE(str2 == "hello preallocated");
    }

    GIVEN("map") {
        using KVMap = std::map<int, int, std::less<>, Allocator<std::pair<const int, int>>>;
        {
            KVMap map = {
                    {0, 1},
                    {1, 2}
            };

            REQUIRE(map[0] == 1);
            REQUIRE(map[1] == 2);
        }

        {
            Allocator<std::pair<const int, int>> allocator(GetResource());
            KVMap map(allocator);

            map[0] = 1;
            map[1] = 2;

            REQUIRE(map[0] == 1);
            REQUIRE(map[1] == 2);
        }

        {
            Allocator<std::pair<const int, int>> allocator(GetResource());
            KVMap map(allocator);

            map.insert(std::make_pair(0, 1));
            map.insert(std::make_pair(1, 2));

            REQUIRE(map[0] == 1);
            REQUIRE(map[1] == 2);
        }
    }

    Terminate();
}


SCENARIO( "allocator make shared_ptr") {

  class Base {
   public:
    virtual ~Base() {
      std::cout << "base" << std::endl;
    }
  };

  class Foo: public Base {
   public:
    Foo() = default;
    ~Foo() final {
      std::cout << "foo" << std::endl;
    }

   private:
    int v = 100;
  };

  SetupRuntime();

  {
    std::vector<std::shared_ptr<Base>, AsyncRuntime::Allocator<std::shared_ptr<Base>>> v((AsyncRuntime::Allocator<std::shared_ptr<Base>>(GetResource())));

    auto foo_ptr = AsyncRuntime::make_shared_ptr<Foo>(GetResource());
    v.push_back(foo_ptr);
  }


  Terminate();
}

