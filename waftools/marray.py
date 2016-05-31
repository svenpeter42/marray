from waflib import Task, Options, Configure, TaskGen, Logs, Build, Utils, Errors
from waflib.TaskGen import feature, before_method
from waflib.Configure import conf

TEST_INIT_LIST = '''
    #include <initializer_list>
    void dummy(std::initializer_list<int> test) {

    }
    int main()
    {
        dummy({0,1,2});
        return 0;
    }
'''

TEST_VARIADIC = '''
  bool some_function(int dummy) { }
  template<typename... Args> inline void pass(Args&&... args) { }

  template<typename... Args> inline void expand(Args&&... args) {
    pass( some_function(args)... );
  }
  int main()
  {
    expand(1, 2, 3);
  }
'''

TEST_ALIAS = '''
template<typename A, typename B>
void dummy(A dummy0, B dummy1) { }

template<class C> using dummy_alias = dummy<C, bool>;
int main()
{
  dummy_alias<bool, bool>(false, false);
}
'''

@conf
def check_marray(cfg):
    cfg.check_cxx(
        header_name='cpuid.h',
        define_name='HAVE_CPP11_INITIALIZER_LISTS',
        msg='Checking if the compiler supports std::initializer_list',
        fragment=TEST_INIT_LIST,
        mandatory=False)

    cfg.check_cxx(
        header_name='cpuid.h',
        define_name='HAVE_CPP11_VARIADIC_TEMPLATES',
        msg='Checking if the compiler supports variadic templates',
        fragment=TEST_VARIADIC,
        mandatory=False)

    cfg.check_cxx(
        header_name='cpuid.h',
        define_name='HAVE_CPP11_TEMPLATE_ALIASES',
        msg='Checking if the compiler supports template aliases',
        fragment=TEST_ALIAS,
        mandatory=False)