#pragma once

#include <string>

#include <iostream>
#include <type_traits>
#include <string>
#include <cereal/details/util.hpp>

namespace nu {
namespace utils {

    #define DEBUG_P(message) nu::utils::debug_print(message, __FILE__, __LINE__, __func__)
    #define DEBUG_P_STARTS() nu::utils::debug_print_starts(__FILE__, __LINE__, __func__)
    #define DEBUG_P_ENDS() nu::utils::debug_print_ends()

    static void debug_print(std::string message, const char* file, int line, const char* func) {
        std::cout << "****** " << file << ":" << line << " " << func << " ******\n" << message << "****** ENDS ******" << std::endl;
    }

    static void debug_print_starts(const char* file, int line, const char* func) {
        std::cout << "****** " << file << ":" << line << " " << func << " ******" << std::endl;
    }

    static void debug_print_ends() {
        std::cout << "****** ENDS ******" << std::endl;
    }

    class IPUtils {
    public:
        static inline std::string uint32_to_str(int32_t ip) {
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
            std::cout << "printing function signature..." << std::endl;
            std::cout << "\t" + get_type_str<ReturnType>() + " fn_ptr";
            std::cout << "(";
            (print_type<Args>(), ...);
            std::cout << ")";
            std::cout << std::endl;
        }

        static std::string get_signature() {
            std::string signature = "printing function signature...\n";
            signature += "\t" + get_type_str<ReturnType>() + " fn_ptr(";
            ((signature += get_type_str<Args>() + ", "), ...);
            if constexpr (sizeof...(Args) > 0) {
                signature.erase(signature.length() - 2);
            }
            signature += ")";
            return signature;
        }

      private:
        template <typename T>
        static void print_type() {
            std::cout << get_type_str<T>() << ", ";
        }

        template <typename T>
        static std::string get_type_str() {
            return cereal::util::demangle(typeid(T).name());
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
