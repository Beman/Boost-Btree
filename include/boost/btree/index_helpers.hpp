//  boost/btree/index_helpers.hpp  -----------------------------------------------------//

//  Copyright Beman Dawes 2013

//  Distributed under the Boost Software License, Version 1.0.
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef BOOST_BTREE_INDEX_HELPERS_HPP
#define BOOST_BTREE_INDEX_HELPERS_HPP

#ifdef _MSC_VER
# pragma warning(push)
# pragma warning(disable : 4996)   // equivalent to -D_SCL_SECURE_NO_WARNINGS
#endif

#include <boost/config/warning_disable.hpp>

#include <boost/btree/detail/config.hpp>
#include <boost/btree/helpers.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/btree/support/size_t_codec.hpp>
#include <boost/btree/support/string_view.hpp>
#include <boost/assert.hpp>
#include <cstring>

namespace boost
{
namespace btree
{

//--------------------------------------------------------------------------------------//
//                                   index traits                                       //
//--------------------------------------------------------------------------------------//

//------------------  defaults for all fixed length data types  ------------------------//

  template <class T>
  struct index_reference { typedef const T&  type; };

  // given the proxy, return size required to serialize, including any overhead bytes
  template <class T>
  inline std::size_t index_serialize_size(const T&)     {return sizeof(T);}

  //// given pointer to flat file, return size of element, including any overhead bytes
  //template <class T>
  //inline std::size_t index_flat_size(const char*)  {return sizeof(T);} 

  template <class T>
  inline void index_serialize(const T& x, char** flat, std::size_t sz)
  { 
    BOOST_ASSERT(flat);
    BOOST_ASSERT(*flat);
    BOOST_ASSERT_MSG(sz == sizeof(T),
      "btree index_serialize: size error; did you mean to uses a varaible-size type?");
    std::memcpy(*flat, reinterpret_cast<const char*>(&x), sz);
    *flat += sz;
  }

  template <class T>
  inline typename index_reference<T>::type index_deserialize(const char** flat)
  {
    BOOST_ASSERT(flat);
    BOOST_ASSERT(*flat);
    const char* p = *flat;
    *flat += sizeof(T);
    return *reinterpret_cast<const T*>(p);
  }


  //------------------  string_view (i.e. C++ style string) traits  --------------------//

  template <>
  struct index_reference<boost::string_view>
    { typedef const boost::string_view  type; };

  // given the proxy, return size required to serialize, including any overhead bytes
  inline std::size_t index_serialize_size(const boost::string_view& sv)
  {
    typedef btree::support::size_t_codec codec;
    return sv.size() + codec::encoded_size(sv.size());
  }

  //// given pointer to flat file, return size of element, including any overhead bytes
  //inline std::size_t index_flat_size(const char* flat)
  //{
  //  typedef btree::support::size_t_codec codec;
  //  std::pair<std::size_t, std::size_t> dec = codec::decode(flat);
  //  return dec.first + dec.second;
  //}

  inline void index_serialize(const boost::string_view& sv, char** flat, std::size_t sz)
  { 
    typedef btree::support::size_t_codec codec;
    BOOST_ASSERT(flat);
    BOOST_ASSERT(*flat);
    BOOST_ASSERT(sz > sv.size()); // even a null string needs a byte to store size
    std::size_t size_size = sz - sv.size();
    codec::encode(sv.size(), *flat, size_size);  //TODO: encode should not be responsible
                                                 // for the actual memcpy to *flat ?
    *flat += size_size;
    std::memcpy(*flat, sv.data(), sv.size());
    *flat += sv.size();
  }

  template <>
  inline index_reference<boost::string_view>::type
    index_deserialize<boost::string_view>(const char** flat)
  {
    typedef btree::support::size_t_codec codec;
    BOOST_ASSERT(flat);
    BOOST_ASSERT(*flat);
    std::pair<std::size_t, std::size_t> dec = codec::decode(*flat);
    *flat += dec.second;  // size of the size prefix
    const char* p = *flat;
    *flat += dec.first;   // size of the string
    return boost::string_view(p, dec.first);
  }

}  // namespace btree
}  // namespace boost

#ifdef _MSC_VER
#  pragma warning(pop) 
#endif

#endif // BOOST_BTREE_INDEX_HELPERS_HPP
