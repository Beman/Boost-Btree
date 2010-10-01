//  stl_equivalence_test.hpp  ----------------------------------------------------------//

//  Copyright Beman Dawes 2010

//  Distributed under the Boost Software License, Version 1.0.
//  http://www.boost.org/LICENSE_1_0.txt

//  See http://www.boost.org/libs/btree for documentation.

#include <boost/btree/map.hpp>
#include <map>
#include <boost/random.hpp>
#include <boost/cstdint.hpp>
#include <boost/btree/detail/timer.hpp>
#include <boost/btree/detail/fixstr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdlib>  // for atol()
#include <cstring>  // for strcmp(), strncmp()
#include <ctime>    // for time()
#include <stdexcept>
#include <utility>

using std::atol;
using std::strcmp;
using std::strncmp;
using std::cout;
using std::endl;
using std::pair;
using std::runtime_error;
using std::string;
using boost::lexical_cast;
namespace fs = boost::filesystem;

namespace
{
  string command_args;
  string path_prefix("stl_equivalence");
  string path_str;
  boost::int32_t max = 10000;
  boost::int32_t min = 10;
  boost::int32_t low = 0;
  boost::int32_t high = 0;
  boost::int32_t cycles = 3;
  boost::int32_t seed = 1;
  boost::int32_t erase_seed = 0;
  boost::int32_t insert_seed = 0;
  boost::int32_t page_sz = 128;
  boost::int32_t cache_sz = 2;
  string restart;
  bool verbose = false;

  string dump_id(boost::lexical_cast<string>(std::time(0))); 

  boost::rand48  insert_rng;
  boost::rand48  erase_rng;

  boost::uint64_t insert_success_count = 0;
  boost::uint64_t insert_fail_count = 0;
  boost::uint64_t erase_success_count = 0;
  boost::uint64_t erase_fail_count = 0;
  boost::uint64_t iterate_forward_count = 0;
  boost::uint64_t iterate_backward_count = 0;
  boost::uint64_t find_success_count = 0;
  boost::uint64_t find_fail_count = 0;
  boost::uint64_t lower_bound_exist_count = 0;
  boost::uint64_t lower_bound_may_exist_count = 0;
  boost::uint64_t upper_bound_exist_count = 0;
  boost::uint64_t upper_bound_may_exist_count = 0;
  boost::uint32_t cycles_complete = 0;

  typedef boost::btree::btree_map<boost::int32_t, boost::int32_t> bt_type;
  bt_type bt;

  typedef std::map<boost::int32_t, boost::int32_t>  stl_type;
  stl_type stl;

  typedef stl_type::value_type value_type;

  typedef boost::variate_generator<boost::rand48&,
    boost::uniform_int<boost::int32_t> > keygen_t;

  void report_counts()
  {
    cout << "\nCumulative counts:\n"
         << "  insert, return second true  " << insert_success_count << '\n'
         << "  insert, return second false " << insert_fail_count << '\n'
         << "  erase, return > 0           " << erase_success_count << '\n'
         << "  erase, return == 0          " << erase_fail_count << '\n'
         << "  iterate forward             " << iterate_forward_count << '\n'
         << "  iterate backward            " << iterate_backward_count << '\n'
         << "  find, return iterator       " << find_success_count << '\n'
         << "  find, return end iterator   " << find_fail_count << '\n'
         << "  lower_bound, key exists     " << lower_bound_exist_count << '\n'
         << "  lower_bound, key may exist  " << lower_bound_may_exist_count << '\n'
         << "  lower_bound, key exists     " << lower_bound_exist_count << '\n'
         << "  lower_bound, key may exist  " << lower_bound_may_exist_count << '\n'
         << "  total (i.e. sum the above)  " << insert_success_count
           +insert_fail_count+erase_success_count+erase_fail_count+iterate_forward_count
           +iterate_backward_count+find_success_count+find_fail_count
           +lower_bound_exist_count+lower_bound_may_exist_count
           +upper_bound_exist_count+upper_bound_may_exist_count
         << '\n' 
         << "  cycles complete             " << cycles_complete  << '\n'
         << "  current size()              " << stl.size()
         << endl
         ;
  }

  //  dump_state  ----------------------------------------------------------------------//

  void dump_state()
  {
    cout << "\nwriting state files...\n";

    //  program state
    fs::path p = dump_id + ".state";
    cout << "  dumping program state to " << p << '\n';
    std::ofstream stateout(p.c_str());
    if (!stateout)
      throw runtime_error(string("Could not open ") + p.string());

    stateout << max << '\n'
             << min << '\n'
             << low << '\n'
             << high << '\n'
             << cache_sz << '\n'
             << insert_rng << '\n'
             << erase_rng << '\n'
             ;
    stateout.close();

    //  bt state
    p = dump_id + ".btr"; 
    cout << "  copying " << path_str << " to " << p << '\n';
    fs::copy_file(p, path_str, fs::copy_option::overwrite_if_exists);

    //  stl state
    p = dump_id + ".stl";
    cout << "  dumping stl::map data to " << p << '\n';
    std::ofstream stlout(p.c_str());
    if (!stlout)
      throw runtime_error(string("Could not open ") + p.string());
    for (stl_type::const_iterator it = stl.begin(); it != stl.end(); ++it)
      stlout << it->first << ',' << it->second << '\n';

    cout << "  writing state files complete\n";
  }

  //  load_state  ----------------------------------------------------------------------//

  void load_state()
  {
    cout << "\nreading state files...\n";

    //  program state
    fs::path p = restart + ".state";
    cout << "  loading program state from " << p << '\n';
    std::ifstream statein(p.c_str());
    if (!statein)
      throw runtime_error(string("Could not open ") + p.string());

    statein >> max
            >> min
            >> low
            >> high
            >> cache_sz
            >> insert_rng
            >> erase_rng
            ;
    statein.close();

    //  bt state
    p = restart + ".btr"; 
    cout << "  copying " << p << " to " << path_str << '\n';
    fs::copy_file(p, path_str, fs::copy_option::overwrite_if_exists);

    //  stl state
    p = restart + ".stl";
    cout << "  loading stl::map data from " << p << '\n';
    std::ifstream stlin(p.c_str());
    if (!stlin)
      throw runtime_error(string("Could not open ") + p.string());

    while (!stlin.eof())
    {
      stl_type::key_type key;
      stlin >> key;
      stl.insert(std::make_pair(key, key));
    }

    stlin.close();

    cout << "  reading state files complete\n";
  }

  //  insert test  ---------------------------------------------------------------------//

  void insert_test(keygen_t& insert_key)
  {
    cout << "insert test..." << endl;
    while (stl.size() < static_cast<stl_type::size_type>(max))
    {
      value_type element(insert_key(),0);
      element.second = element.first;

      pair<stl_type::iterator, bool> stl_result = stl.insert(element);
      pair<bt_type::const_iterator, bool> bt_result = bt.insert(element);

      if (stl_result.second != bt_result.second)
      {
        cout << "failure inserting element " << element.first << endl;
        throw runtime_error("insert: stl_result.second != bt_result.second");
      }

      if (stl_result.second)
        ++insert_success_count;
      else
        ++insert_fail_count;
    }
    if (stl.size() != bt.size())
    {
      cout << "stl.size() " << stl.size() << " != bt.size() " << bt.size() << endl;
      throw runtime_error("insert: size check failure");
    }
    cout << "  insert test complete, size() = " << stl.size() << endl;
  }
  //  iteration test  ------------------------------------------------------------------//

  void iteration_test()
  {
    cout << "iteration test..." << endl;
    stl_type::const_iterator stl_itr = stl.begin();
    bt_type::const_iterator bt_itr = bt.begin();

    for (; stl_itr != stl.end() && bt_itr != bt.end(); ++stl_itr, ++bt_itr)
    {
      if (stl_itr->first != bt_itr->first)
      {
        cout << "stl_itr->first " << stl_itr->first << " != "
              << "bt_itr->first " << bt_itr->first << endl;
        throw runtime_error("iteration: first check failure");
      }
      if (stl_itr->second != bt_itr->second)
      {
        cout << "stl_itr->second " << stl_itr->second << " != "
              << "bt_itr->second " << bt_itr->second << endl;
        throw runtime_error("iteration: second check failure");
      }
      ++iterate_forward_count;
    }

    if (stl_itr != stl.end())
      throw runtime_error("iteration: bt at end() but stl not at end()");
    if (bt_itr != bt.end())
      throw runtime_error("iteration: stl at end() but bt not at end()");
    cout << "  iteration test complete" << endl;
  }

  //  backward iteration test  ---------------------------------------------------------//

  void backward_iteration_test()
  {
    cout << "backward iteration test..." << endl;
    stl_type::const_iterator stl_itr = stl.end();
    bt_type::const_iterator bt_itr = bt.end();

    do
    {
      --stl_itr;
      --bt_itr;
      if (stl_itr->first != bt_itr->first)
      {
        cout << "stl_itr->first " << stl_itr->first << " != "
              << "bt_itr->first " << bt_itr->first << endl;
        throw runtime_error("backward iteration: first check failure");
      }
      if (stl_itr->second != bt_itr->second)
      {
        cout << "stl_itr->second " << stl_itr->second << " != "
              << "bt_itr->second " << bt_itr->second << endl;
        throw runtime_error("backward iteration: second check failure");
      }
      ++iterate_backward_count;
    } while (stl_itr != stl.begin() && bt_itr != bt.begin());

    if (stl_itr != stl.begin())
      throw runtime_error("iteration: bt at begin() but stl not at begin()");
    if (bt_itr != bt.begin())
      throw runtime_error("iteration: stl at begin() but bt not at begin()");
    cout << "  backward iteration complete" << endl;
  }

  //  erase test  ----------------------------------------------------------------------//

  void erase_test(keygen_t& erase_key)
  {
    cout << "erase test..." << endl;
    while (stl.size() > static_cast<stl_type::size_type>(min))
    {
      boost::int32_t k = erase_key();
      stl_type::size_type stl_result = stl.erase(k);
      bt_type::size_type bt_result = bt.erase(k);

      if (stl_result != bt_result)
      {
        cout << "stl_result " << stl_result << " != bt_result " << bt_result << endl;
        throw runtime_error("erase: result failure");
      }

      if (stl_result)
        ++erase_success_count;
      else
        ++erase_fail_count;
    }
    if (stl.size() != bt.size())
    {
      cout << "stl.size() " << stl.size() << " != bt.size() " << bt.size() << endl;
      throw runtime_error("erase: size check failure");
    }
    cout << "  erase test complete, size() = " << stl.size() << endl;
  }

  //  find test  -----------------------------------------------------------------------//

  void find_test()
  {
    cout << "find test..." << endl;

    boost::minstd_rand find_rng;
    boost::uniform_int<boost::int32_t> n_dist(low, high);
    boost::variate_generator<boost::minstd_rand&, boost::uniform_int<boost::int32_t> >
      find_key(find_rng, n_dist);

    stl_type::const_iterator stl_itr, stl_result;
    bt_type::const_iterator bt_result;

    for (stl_itr = stl.begin(); stl_itr != stl.end(); ++stl_itr)
    {
      //  test with key that exists
      stl_result = stl.find(stl_itr->first);
      bt_result = bt.find(stl_itr->first);

      if (bt_result == bt.end())
      {
        cout << "for key " << stl_itr->first << ", bt.find() return bt.end()" << endl;
        throw runtime_error("find: failed to find key");
      }

      if (stl_result->first != bt_result->first)
      {
        cout << "stl_result->first " << stl_result->first << " != "
              << "bt_result->first " << bt_result->first << endl;
        throw runtime_error("find: first check failure");
      }
      if (stl_result->second != bt_result->second)
      {
        cout << "stl_result->second " << stl_result->second << " != "
              << "bt_result->second " << bt_result->second << endl;
        throw runtime_error("find: second check failure");
      }
      ++find_success_count;

      //  test with key that may or may not exist  
      find_rng.seed(stl_result->first);
      boost::int32_t k = find_key();

      stl_result = stl.find(k);
      bt_result = bt.find(k);

      if (stl_result == stl.end() && bt_result != bt.end())
      {
        cout << "stl find()==end(), but bt finds " << k << endl;
        throw runtime_error("find: results inconsistent");
      }
      if (stl_result != stl.end() && bt_result == bt.end())
      {
        cout << "bt find()==end(), but stl finds " << k << endl;
        throw runtime_error("find: results inconsistent");
      }
      if (stl_result == stl.end())
        ++find_fail_count;
      else if (bt_result->first == k)
        ++find_success_count;
      else
      {
        cout << "bt finds " << bt_result->first << ", but should be " << k << endl;
        throw runtime_error("find: wrong iterator");
      }
    }

    cout << "  find test complete" << endl;
  }

  //  lower_bound test  ----------------------------------------------------------------//

  void lower_bound_test()
  {
    cout << "lower_bound test..." << endl;

    boost::minstd_rand lower_bound_rng;
    boost::uniform_int<boost::int32_t> n_dist(low, high);
    boost::variate_generator<boost::minstd_rand&, boost::uniform_int<boost::int32_t> >
      lower_bound_key(lower_bound_rng, n_dist);

    stl_type::const_iterator stl_itr, stl_result;
    bt_type::const_iterator bt_result;

    for (stl_itr = stl.begin(); stl_itr != stl.end(); ++stl_itr)
    {
      //  test with key that exists
      stl_result = stl.lower_bound(stl_itr->first);
      bt_result = bt.lower_bound(stl_itr->first);

      if (stl_result == stl.end())
      {
        cout << "for key " << stl_itr->first << ", stl.lower_bound() return stl.end()" << endl;
        throw runtime_error("lower_bound: unexpected stl.end()");
      }

      if (bt_result == bt.end())
      {
        cout << "for key " << stl_itr->first << ", bt.lower_bound() return bt.end()" << endl;
        throw runtime_error("lower_bound: unexpected bt.end()");
      }

      if (stl_result->first != bt_result->first)
      {
        cout << "stl_result->first " << stl_result->first << " != "
              << "bt_result->first " << bt_result->first << endl;
        throw runtime_error("lower_bound: first check failure");
      }
      if (stl_result->second != bt_result->second)
      {
        cout << "stl_result->second " << stl_result->second << " != "
              << "bt_result->second " << bt_result->second << endl;
        throw runtime_error("lower_bound: second check failure");
      }
      ++lower_bound_exist_count;

      //  test with key that may or may not exist  
      lower_bound_rng.seed(stl_result->first);
      boost::int32_t k = lower_bound_key();

      stl_result = stl.lower_bound(k);
      bt_result = bt.lower_bound(k);

      if (stl_result == stl.end() && bt_result != bt.end())
      {
        cout << "stl lower_bound()==end(), but bt lower_bounds " << k << endl;
        throw runtime_error("lower_bound: results inconsistent");
      }
      if (stl_result != stl.end() && bt_result == bt.end())
      {
        cout << "bt lower_bound()==end(), but stl lower_bounds " << k << endl;
        throw runtime_error("lower_bound: results inconsistent");
      }
      if (stl_result != stl.end() && bt_result != bt.end())
      {
        if (stl_result->first != bt_result->first)
        {
          cout << "stl_result->first " << stl_result->first << " != "
                << "bt_result->first " << bt_result->first << endl;
          throw runtime_error("lower_bound may exist: first check failure");
        }
        if (stl_result->second != bt_result->second)
        {
          cout << "stl_result->second " << stl_result->second << " != "
                << "bt_result->second " << bt_result->second << endl;
          throw runtime_error("lower_bound may exist: second check failure");
        }
      }
      ++lower_bound_may_exist_count;
    }

    cout << "  lower_bound test complete" << endl;
  }
  
  //  upper_bound test  ----------------------------------------------------------------//

  void upper_bound_test()
  {
    cout << "upper_bound test..." << endl;

    boost::minstd_rand upper_bound_rng;
    boost::uniform_int<boost::int32_t> n_dist(low, high);
    boost::variate_generator<boost::minstd_rand&, boost::uniform_int<boost::int32_t> >
      upper_bound_key(upper_bound_rng, n_dist);

    stl_type::const_iterator stl_itr, stl_result;
    bt_type::const_iterator bt_result;

    for (stl_itr = stl.begin(); stl_itr != stl.end(); ++stl_itr)
    {
      //  test with key that exists
      stl_result = stl.upper_bound(stl_itr->first);
      bt_result = bt.upper_bound(stl_itr->first);

      if (stl_result == stl.end() && bt_result != bt.end())
      {
        cout << "stl upper_bound()==end(), but bt upper_bounds " << bt_result->first
             << " for key " << stl_itr->first << endl;
        throw runtime_error("upper_bound: results inconsistent");
      }
      if (stl_result != stl.end() && bt_result == bt.end())
      {
        cout << "bt upper_bound()==end(), but stl upper_bounds " << stl_result->first
             << " for key " << stl_itr->first << endl;
        throw runtime_error("upper_bound: results inconsistent");
      }
      if (stl_result != stl.end() && bt_result != bt.end())
      {
        if (stl_result->first != bt_result->first)
        {
          cout << "stl_result->first " << stl_result->first << " != "
                << "bt_result->first " << bt_result->first << endl;
          throw runtime_error("upper_bound key exists: first check failure");
        }
        if (stl_result->second != bt_result->second)
        {
          cout << "stl_result->second " << stl_result->second << " != "
                << "bt_result->second " << bt_result->second << endl;
          throw runtime_error("upper_bound key exists: second check failure");
        }
      }
      ++upper_bound_exist_count;

      if (stl_result == stl.end())  // upper_bound of last element is end()
        continue;

      //  test with key that may or may not exist  
      upper_bound_rng.seed(stl_result->first);
      boost::int32_t k = upper_bound_key();

      stl_result = stl.upper_bound(k);
      bt_result = bt.upper_bound(k);

      if (stl_result == stl.end() && bt_result != bt.end())
      {
        cout << "stl upper_bound()==end(), but bt upper_bounds " << bt_result->first
             << " for k " << k << endl;
        throw runtime_error("upper_bound: results inconsistent");
      }
      if (stl_result != stl.end() && bt_result == bt.end())
      {
        cout << "bt upper_bound()==end(), but stl upper_bounds " << stl_result->first
             << " for k " << k << endl;
        throw runtime_error("upper_bound: results inconsistent");
      }
      if (stl_result != stl.end() && bt_result != bt.end())
      {
        if (stl_result->first != bt_result->first)
        {
          cout << "stl_result->first " << stl_result->first << " != "
                << "bt_result->first " << bt_result->first << endl;
          throw runtime_error("upper_bound may exist: first check failure");
        }
        if (stl_result->second != bt_result->second)
        {
          cout << "stl_result->second " << stl_result->second << " != "
                << "bt_result->second " << bt_result->second << endl;
          throw runtime_error("upper_bound may exist: second check failure");
        }
      }
      ++upper_bound_may_exist_count;
    }

    cout << "  upper_bound test complete" << endl;
  }


  //  run test cycles  -----------------------------------------------------------------//

  void tests()
  {

    insert_rng.seed(insert_seed);
    erase_rng.seed(erase_seed);

    boost::uniform_int<boost::int32_t> n_dist(low, high);
    keygen_t insert_keygen(insert_rng, n_dist);
    keygen_t erase_keygen(erase_rng, n_dist);

    path_str = path_prefix + ".btr";

    bt.open(path_str, boost::btree::flags::truncate, page_sz);
    bt.max_cache_pages(cache_sz);  // small cache to incease stress

    boost::btree::run_timer total_times(3);
    boost::btree::run_timer cycle_times(3);

    for (boost::int32_t cycle = 1; cycle <= cycles; ++cycle)
    {
      cout << "\nBeginning cycle " << cycle << " ..." << endl;
      cycle_times.start();

      insert_test(insert_keygen);
      iteration_test();
      backward_iteration_test();
      find_test();
      lower_bound_test();
      upper_bound_test();
      erase_test(erase_keygen);
      iteration_test();
      backward_iteration_test();
      find_test();
      lower_bound_test();
      upper_bound_test();

      ++cycles_complete;
      report_counts();
      dump_state();
      cout << "cycle " << cycle << " complete" << endl;
      cout << " ";
      cycle_times.stop();
      cycle_times.report();
    }

    //  cout << bt.manager();
    cout << "\n total time: ";
  }

}  // unnamed namespace

int main(int argc, char *argv[])
{
  for (int a = 0; a < argc; ++a)
  {
    command_args += argv[a];
    if (a != argc-1)
      command_args += ' ';
  }

  cout << "command line arguments: " << command_args << '\n';;

  for (; argc > 1; ++argv, --argc) 
  {
    if (*argv[1] != '-')
      path_prefix = argv[1];
    else
    {
      if (strncmp(argv[1]+1, "max=", 4) == 0)
        max = atol(argv[1]+5);
      else if (strncmp(argv[1]+1, "min=", 4) == 0)
        min = atol(argv[1]+5);
      else if (strncmp(argv[1]+1, "low=", 4) == 0)
        low = atol(argv[1]+5);
      else if (strncmp(argv[1]+1, "high=", 5) == 0)
        high = atol(argv[1]+6);
      else if (strncmp(argv[1]+1, "cycles=", 7) == 0)
        cycles = atol(argv[1]+8);
      else if (strncmp(argv[1]+1, "seed=", 5) == 0)
        seed = atol(argv[1]+6);
      else if (strncmp(argv[1]+1, "page=", 5) == 0)
        page_sz = atol(argv[1]+6);
      else if (strncmp(argv[1]+1, "cache=", 6) == 0)
        cache_sz = atol(argv[1]+7);
      else if (strncmp(argv[1]+1, "restart=", 8) == 0)
        restart = argv[1]+9;
      else if (strcmp(argv[1]+1, "v") == 0)
        verbose = true;
      else
      {
        cout << "Error - unknown option: " << argv[1] << "\n\n";
        argc = -1;
        break;
      }
    }
  }

  if (argc < 2) 
  {
    cout << "Usage: stl_equivalence_test [Options]\n"
      "The argument n specifies the number of test cases to run\n"
      "Options:\n"
      "   path-prefix  Test files path-prefix; default '" << path_prefix << "'\n"
      "                Two files will be created; path-prefix.btr and path-prefix.stl\n"
      "   -max=#       Maximum number of test elements; default " << max << "\n"
      "   -min=#       Minimum number of test elements; default " << min << "\n"
      "   -low=#       Random key distribution low value; default 0\n"
      "   -high=#      Random key distribution high value; default max*2.\n"
      "                (high-low) must be >max, so that max is reached\n"
      "   -cycles=#    Cycle tests specified number of times; default " << cycles << "\n"
      "                -cycles=0 causes tests to cycle forever\n"
      "   -seed=#      Seed for random number generator; default"  << seed << "\n"
      "   -page=#      Page size (>=128); default " << page_sz << "\n"
      "                Small page sizes increase stress\n"
      "   -cache=#     Cache size; default " << cache_sz << " pages\n"
      "   -restart=dump-id  Restart using dump files identified by dump-id\n"
      "   -v           Verbose output statistics\n"
      "\n    Each test cycle inserts the same random values into both a btree_map\n"
      "and a std::map until the maximum number of elements is reached.\n"
      "The same sequence of random elements will then be erased, until the minimum\n"
      "number of elements is reached. The btree, std::map, and program state are\n"
      "copied to files, and the cycle ends.\n"
      "    At the maximum and minimum points of each cycle, forward iteration,\n"
      "backward iteration, find, lower_bound, and upper_bound tests are run\n"
      "against both containers. If results are not identical, the program\n"
      "issues an error message and returns 1.\n"
      "    The -restart argument causes the btree, std::map, and program to be\n"
      "initialized to the state save in the dump files from the prior run.\n"
      ;
  }

  if (argc == -1)
    return 1;

  if (high == 0)
    high = max * 2;

  if ((high-low) <= max)
  {
    cout << "Error: (high-low) must be greater than max\n";
    return 1;
  }

  insert_seed = seed;
  erase_seed = seed;

  cout << "starting tests with:\n"
       << "  path_prefix = " << path_prefix << '\n'
       << "  max = " << max << '\n'
       << "  min = " << min << '\n'
       << "  lo = " << low  << '\n'
       << "  hi = " << high  << '\n'
       << "  cycles = " << cycles << '\n'
       << "  insert seed = " << insert_seed << '\n'
       << "  erase seed = " << erase_seed << '\n'
       << "  page size = " << page_sz << '\n'
       << "  max cache pages = " << cache_sz << "\n";

  try
  {
    tests();
  }

  catch (const std::runtime_error& ex)
  {
    cout << "\n*************** exception  ******************\n"
         << ex.what() << endl;
    return 1;
  }
  catch (...)
  {
    cout << "\n*************** exception  ******************\n"
         << "of a type not derived from std::runtime_error" << endl;
    return 1;
  }

  cout << "all test cycles complete" << endl;
  return 0;
}
