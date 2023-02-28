#include <memory>

//  Any : 可以代表并接收任意类型
class Any
{
public:
    Any() = default;
    ~Any() = default;
    Any(const Any &) = delete; //  因为unique_ptr
    Any &operator=(const Any &) = delete;
    Any(Any &&) = default;
    Any &operator=(Any &&) = default;

    //  使得Any接收任意类型data,并存入Derived对象 由base管理.
    template <typename T>
    Any(T data) : base_(std::make_unique<Derive<T>>())
    {
    }

    //  user通过cast将Any对象里存储的data提取出来.
        //  user需要自己确定 其传入的T和Derive<T>是一致的
    template <typename T>
    T cast()
    {
        //  从base_找到所指向的Derive. 从其中取出data_
        //  base pointer --> derive pointer only when base指针确实指向derive
        Derive<T> *pd = dynamic_cast<Derive<T>*>(base_.get());
        if(pd == nullptr)
            throw "type is unmatch"!;
        return pd->data();
    }

private:
    class Base
    {
    public:
        virtual ~Base() = default;
    };

    //  Derive : 用于存储任意类型的data
    //  对于不同类型的data , 也即不同类型的T , Derive的类型是不同的.(因为Derive+T合在一起是一个完整的类型)
    //  因此Any里面需要有一个类型可以统一的接收这些Dervie<T>
    //  那么很显然 , 这个类型就应当是 Derive<T> 的父类指针. 于是我们令他继承一个父类Base
    template <typename T>
    class Derive : public Base
    {
    public:
        Derive(T data) : data_(data)
        {
        }
        T data() { return data_; }
    private:
        T data_;
    };

    std::unique_ptr<Base> base_;
};
