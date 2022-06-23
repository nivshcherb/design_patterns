/* -------------------------------------------------------------------------- */
/* singleton.hpp                                                              */
/* -------------------------------------------------------------------------- */

#ifndef __DP_SINGLETON_HPP__
#define __DP_SINGLETON_HPP__

/* -------------------------------------------------------------------------- */
/* singleton                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Wrapper class allowing only a single instance to exist at a time.
 * @tparam C Instance class. Singleton requires a constructor with no
 * arguments. In addition, it is recommended to have C non-copyable and with
 * access to the constructor blocked for everyone exept Singleton.
 */
template<class C>
class Singleton
{
public:
    /**
     * @brief Destroy the Singleton object.
     */
    ~Singleton()                                    = default;

    // non-copyable
    Singleton(const Singleton& other_)              = delete;
    Singleton& operator=(const Singleton& other_)   = delete;

    /**
     * @brief Get the Instance object.
     * @return C& The single instance of class C.
     */
    static inline C& GetInstance();

private:
    /**
     * @brief Construct a new Singleton object.
     */
    explicit Singleton()                            = default;

    /* members -------------------------------------------------------------- */
    C m_data;

};

/* implementation ----------------------------------------------------------- */

template<class C>
inline C& Singleton<C>::GetInstance()
{
    static Singleton<C> s_instance;
    return s_instance.m_data;
}

/* -------------------------------------------------------------------------- */
#endif /* __DP_SINGLETON_HPP__ */