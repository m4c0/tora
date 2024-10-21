export module args;
import jute;

// Super simple class for "iterating/parsing" command-line arguments
export class args {
  int m_c;
  char ** m_v;
public:
  args(int argc, char ** argv) : m_c { argc - 1 }, m_v { argv + 1 } {};

  jute::view take() {
    return m_c-- > 0 ? jute::view::unsafe(*m_v++) : jute::view {};
  }
  unsigned take_int() {
    unsigned res {};
    for (auto c : take()) {
      res = res * 10 + (c - '0');
    }
    return res;
  }
};

