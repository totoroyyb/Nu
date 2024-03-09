#pragma once

#include <string>

#include <iostream>
#include <type_traits>
#include <string>
#include <cereal/details/util.hpp>

namespace nu {
namespace utils {
    class IPUtils {
    public:
        static inline std::string int32_to_str(int32_t ip) {
            std::string result;
            for (int i = 0; i < 4; ++i) {
                result.insert(0, std::to_string(ip & 0xFF));
                if (i < 3) {
                    result.insert(0, ".");
                }
                ip >>= 8;
            }
            return result;
        }
    };

    // Base template for a struct to get function signature.
    // We specialize this for function types.
    template <typename T>
    struct function_traits;

    // Specialization for function pointers.
    template <typename ReturnType, typename... Args>
    struct function_traits<ReturnType(*)(Args...)> {
        static void print_signature() {
            std::cout << "Return type: " << typeid(ReturnType).name() << std::endl;
            std::cout << "Parameters: ";
            (print_type<Args>(), ...);
            std::cout << std::endl;
        }

    private:
        template <typename T>
        static void print_type() {
            std::cout << cereal::util::demangle(typeid(T).name()) << " ";
        }
    };

    class TypeUtils {
      public:
        template <typename T>
        static std::string type_name(); // You would implement this function based on your needs or use typeid(T).name() directly.

        template <typename... As>
        static std::string constructor_signature() {
            std::string signature = "Cls(";
            ((signature += type_name<As>() + ", "), ...); // Appends all types to the signature string.
            if constexpr (sizeof...(As) > 0) {
                signature.erase(signature.length() - 2); // Remove the last comma and space.
            }
            signature += ")";
            return signature;
        }

        template <typename Cls>
        static std::string demangled_name() {
            return cereal::util::demangle(typeid(Cls).name());
        }
    };

} // namespace utils
} // namespace nu
