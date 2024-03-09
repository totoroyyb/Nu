#include <algorithm>
#include <cstdint>
#include <iostream>
#include <numeric>
#include <tuple>
#include <vector>

extern "C" {
#include <net/ip.h>
}
#include <runtime.h>

#include "nu/pressure_handler.hpp"
#include "nu/proclet.hpp"
#include "nu/runtime.hpp"

using namespace nu;

class Verifier {
  public:
    bool verify(const std::vector<int> vec_a, const std::vector<int> vec_b,
                const std::vector<int> vec_c) {
      std::cout << "[6] Inside Verifier::verify()..." << std::endl;
      // // Set resource pressure using the mock interface
	    // {
	    //   rt::Preempt p;
	    //   rt::PreemptGuard g(&p);
	    //   get_runtime()->pressure_handler()->mock_set_pressure();
	    // }
	    // // Ensure that the migration happens before the function returns.
	    // delay_us(1000 * 1000);
      // std::cout << "Verifier::verify() starts verifying..." << std::endl;
      for (size_t i = 0; i < vec_a.size(); i++) {
        if (vec_c[i] != vec_a[i] + vec_b[i]) {
          return false;
        }
      }
      std::cout << "[7] Verification returning..." << std::endl;
      return true;
    }
};

class Adder {
 public:
  Adder() : verifier(make_proclet<Verifier>()) {}

  std::vector<int> add(const std::vector<int> vec_a,
                       const std::vector<int> vec_b) {
    std::cout << "[3] Inside Adder::add()..." << std::endl;
    // Set resource pressure using the mock interface
	  {
	    rt::Preempt p;
	    rt::PreemptGuard g(&p);
	    get_runtime()->pressure_handler()->mock_set_pressure();
	  }
	  // Ensure that the migration happens before the function returns.
	  delay_us(1000 * 1000);

    std::cout << "[4] Adder::add() starts adding..." << std::endl;
    std::vector<int> vec_c;
    for (size_t i = 0; i < vec_a.size(); i++) {
      vec_c.push_back(vec_a[i] + vec_b[i]);
    }

    // Set resource pressure using the mock interface
	  {
	    rt::Preempt p;
	    rt::PreemptGuard g(&p);
	    get_runtime()->pressure_handler()->mock_set_pressure();
	  }
	  // Ensure that the migration happens before the function returns.
	  delay_us(1000 * 1000);

    std::cout << "[5] Starts verifying..." << std::endl;
    if (!verifier.run(&Verifier::verify, vec_a, vec_b, vec_c)) {
      std::cout << "Verification failed" << std::endl;
    }

    std::cout << "Adder::add() returning..." << std::endl;
    return vec_c;
  }
 private:
  Proclet<Verifier> verifier;
};

class VecStore {
 public:
  VecStore(const std::vector<int> &a, const std::vector<int> &b)
      : a_(a), b_(b) {
    adder = make_proclet<Adder>();
  }
  std::vector<int> get_vec_a() { return a_; }
  std::vector<int> get_vec_b() { return b_; }

  std::vector<int> add_vec() { 
    std::cout << "[2] Running add_vec()..." << std::endl;
    return adder.run(
        +[](Adder &adder, std::vector<int> a_, std::vector<int> b_) {
          return adder.add(a_, b_);
        },
        a_, b_
    ); 
  }

  std::vector<int> add_vec_nonclosure() {
    std::cout << "Running add_vec_nonclosure()..." << std::endl;
    return adder.run(&Adder::add, a_, b_);
  }

 private:
  std::vector<int> a_;
  std::vector<int> b_;
  Proclet<Adder> adder;
};

void do_work() {
  bool passed = true;

  std::vector<int> a{1, 2, 3, 4};
  std::vector<int> b{5, 6, 7, 8};

  std::cout << "[1] Starting..." << std::endl;

  auto rem_vec = make_proclet<VecStore>(std::forward_as_tuple(a, b));
  auto c = rem_vec.run(&VecStore::add_vec);

  for (size_t i = 0; i < a.size(); i++) {
    if (c[i] != a[i] + b[i]) {
      passed = false;
      break;
    }
  }

  // auto c_nonclosure = rem_vec.run(&VecStore::add_vec_nonclosure);
  // for (size_t i = 0; i < a.size(); i++) {
  //   if (c_nonclosure[i] != a[i] + b[i]) {
  //     passed = false;
  //     break;
  //   }
  // }

  if (passed) {
    std::cout << "Passed" << std::endl;
  } else {
    std::cout << "Failed" << std::endl;
  }
}

int main(int argc, char **argv) {
  return runtime_main_init(argc, argv, [](int, char **) { do_work(); });
}