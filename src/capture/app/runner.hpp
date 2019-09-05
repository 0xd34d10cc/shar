#pragma once

namespace shar {

class Runner {
public:
  Runner();
  ~Runner();

  int run(int argc, char* argv[]);
};

}